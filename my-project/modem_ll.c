#include "callback_timer.h"  //debug
#include "modem_ll.h"
#include "modem_ll_config.h"
#include "sx127x.h"
#include "uart.h"  //debug
#include "util.h"  //debug
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>  //debug
#include <stdbool.h>
#include <stdio.h>   //debug
#include <stdlib.h>  //debug?
#include <sys/types.h>

// function prototypes

struct modem *volatile exti0_modem = NULL;

void exti0_isr(void) {
    // TODO figure out how to demote systick priority
    exti_reset_request(EXTI0);  // must always be first
    // systick_counter_disable();//stop systick from preempting this irq_seen

    // fprintf(fp_uart,"got interrupt\r\n");
    exti0_modem->irq_data = lora_read_reg(exti0_modem, LORA_REG_IRQFLAGS);

    lora_write_reg(exti0_modem, LORA_REG_IRQFLAGS, 0xFF);
    lora_write_reg(exti0_modem, LORA_REG_IRQFLAGS, 0xFF);

    exti0_modem->irq_seen = false;

    switch (exti0_modem->cur_irq_type) {
        case RX_DONE:
            // start_timer(0);

            (*(exti0_modem->rx_callback))(exti0_modem->callback_arg);
            break;
        case TX_DONE:
            // fprintf(fp_uart,"RX to TX time: %li\r\n",stop_timer(0));
            (*(exti0_modem->tx_callback))(exti0_modem->callback_arg);
            break;
        case INVALID:
            //#ifdef DEBUG
            fprintf(fp_uart, "spurious interrupt detected\r\n");
            //#endif
            break;
    }

    // systick_counter_enable();
}

void irq_setup(struct modem *this_modem) {
    // TODO: implement other pins for irq
    // assume EXTI0

    nvic_enable_irq(NVIC_EXTI0_IRQ);  // interrupt on PA0
    nvic_set_priority(NVIC_EXTI0_IRQ, 0);

    exti_select_source(EXTI0, this_modem->hw->irq.port);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI0);
}

void lora_write_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset) {
    // Some assertions to check input data in DEBUG mode

    // Update modem's FIFO address pointer
    ss_clear(this_modem);
    spi_xfer(this_modem->hw->spi_interface, LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(this_modem->hw->spi_interface, offset);
    ss_set(this_modem);

    delay_nops(1000);
    // assume compiler is not good enough to emit an sbi instruction for the digital_writes
    // Write data to FIFO.
    ss_clear(this_modem);
    spi_xfer(this_modem->hw->spi_interface, LORA_REG_FIFO | WRITE_MASK);
    for (uint8_t i = 0; i < len; i++) {
        spi_xfer(this_modem->hw->spi_interface, buf[i]);
    }
    ss_set(this_modem);
}

void lora_read_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset) {
    // Update modem's FIFO address pointer
    ss_clear(this_modem);
    spi_xfer(this_modem->hw->spi_interface, LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(this_modem->hw->spi_interface, offset);
    ss_set(this_modem);

    delay_nops(1000);
    // Read data from FIFO
    ss_clear(this_modem);
    spi_xfer(this_modem->hw->spi_interface, LORA_REG_FIFO);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_xfer(this_modem->hw->spi_interface, 0x00);
    }
    ss_set(this_modem);
}

bool lora_change_mode(struct modem *this_modem, enum lora_mode change_to, bool check_mode) {
    uint8_t mode;
    switch (change_to) {
        case SLEEP:
            this_modem->cur_irq_type = INVALID;
            mode = MODE_SLEEP;
            break;
        case STANDBY:
            this_modem->cur_irq_type = INVALID;
            mode = MODE_STDBY;
            break;
        case RX:
            lora_write_reg(this_modem, REG_DIO_MAPPING_1, 0x00 /* RXDONE */);
            this_modem->cur_irq_type = RX_DONE;
            mode = MODE_RXCON;
            break;
        case TX:
            lora_write_reg(this_modem, REG_DIO_MAPPING_1, 0x40 /* TXDONE */);
            this_modem->cur_irq_type = TX_DONE;
            mode = MODE_TX;
            break;
        default:
            mode = SLEEP;
            break;
    }
    if (check_mode) {
        return lora_write_reg_and_check(this_modem, LORA_REG_OP_MODE, MODE_LORA | mode, true);
    } else {
        lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | mode);
        return true;
    }
}

uint8_t lora_read_reg(struct modem *this_modem, uint8_t reg) {
    // gpio_set(GPIOC,GPIO13);
    // fprintf(fp_uart,"reading from reg: %x\r\n",reg);

    ss_clear(this_modem);
    // gpio_clear(GPIOC,GPIO13);

    spi_xfer(this_modem->hw->spi_interface, reg & 0x7F);
    uint8_t ret = spi_xfer(this_modem->hw->spi_interface, 0);

    ss_set(this_modem);

    return ret;
}

void lora_write_reg(struct modem *this_modem, uint8_t reg, uint8_t val) {
    // fprintf(fp_uart,"writing to reg: %x\r\n",reg);

    ss_clear(this_modem);

    spi_xfer(this_modem->hw->spi_interface, reg | WRITE_MASK);
    spi_xfer(this_modem->hw->spi_interface, val);

    ss_set(this_modem);
}

bool lora_write_reg_and_check(struct modem *this_modem, uint8_t reg, uint8_t val, bool delay) {
    // Write register
    // fprintf(fp_uart,"writing: %x\r\n",val);
    lora_write_reg(this_modem, reg, val);
    // Delay
    if (delay)
        delay_nops(100000);

    // read reg
    uint8_t new_val = lora_read_reg(this_modem, reg);
    // fprintf(fp_uart,"got back: %x\r\n",new_val);

    // Return whether write succeeded
    return val == new_val;
}

void lora_config_modulation(struct modem *this_modem, struct modulation_config *modulation) {
    uint8_t temp = 0;
    temp |= ((uint8_t)modulation->bandwidth) << 4;          // set top nibble for bandwidth
    temp |= (((uint8_t)modulation->coding_rate) + 1) << 1;  // set top 3 bits of bottom nibble for coding rate
    if (!(modulation->header_enabled))
        temp |= 1;  // set bottom bit to indicate header mode
    // fprintf(fp_uart,"reg modem config 1:%x\r\n",temp);
    lora_write_reg(this_modem, LORA_REG_MODEM_CONFIG_1, temp);

    temp = 0;
    temp |= (((uint8_t)modulation->spreading_factor) + 6) << 4;  // set spreading factor
    if (modulation->crc_enabled)
        temp |= (1 << 2);  // set crc enable bit
    lora_write_reg(this_modem, LORA_REG_MODEM_CONFIG_2, temp);

    // set preamble length
    uint8_t bot = (uint8_t)modulation->preamble_length;
    uint8_t top = (uint8_t)(modulation->preamble_length >> 8);

    lora_write_reg(this_modem, LORA_REG_PREAMBLE_MSB, top);
    lora_write_reg(this_modem, LORA_REG_PREAMBLE_LSB, bot);

    // handle settings required if on SF6
    if (modulation->spreading_factor == SF6) {
        lora_write_reg(this_modem, LORA_REG_DETECTION_THRESHOLD, 0x0C);
        lora_write_reg(this_modem, LORA_REG_DETECT_OPTIMIZE, 0x05);
    } else {
        lora_write_reg(this_modem, LORA_REG_DETECTION_THRESHOLD, 0x0A);
        lora_write_reg(this_modem, LORA_REG_DETECT_OPTIMIZE, 0x03);
    }
    // handle data rate optimize
    uint32_t SF_pw = 0x1 << ((uint32_t)((modulation->spreading_factor) + 6));
    uint32_t BW = get_bandwidth(modulation->bandwidth);
    uint32_t symbol_time = (uint32_t)(((double)SF_pw / (double)BW) * 1E6);
#ifdef DEBUG
    fprintf(fp_uart, "symbol time %lu\r\n", symbol_time);
#endif
    uint8_t val = lora_read_reg(this_modem, LORA_REG_MODEM_CONFIG_3);
    if (symbol_time > 16000) {
#ifdef DEBUG
        fprintf(fp_uart, "setting data rate optimize register\r\n");
#endif
        val = val | 0b00001000;
    } else {
        val = val & 0b11110111;
    }

    lora_write_reg(this_modem, LORA_REG_MODEM_CONFIG_3, val);

    // set payload length
    if (modulation->payload_length == 0)
        modulation->payload_length = 1;  // prevent 0 from being written to the register
    lora_write_reg(
        this_modem, LORA_REG_PAYLOAD_LENGTH,
        modulation->payload_length);  // In explicit header mode, this will be overwritten as needed

    // Max out the payload size. This will prevent packet rejection.
    lora_write_reg(this_modem, LORA_REG_MAX_PAYLOAD_LENGTH, MAX_PAYLOAD_LENGTH);

    // inform modem struct of current modulation
    this_modem->modulation = modulation;
}

void seed_random(struct modem *this_modem) {
    uint32_t new_seed = 0;
    lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_RXCON);
    for (uint8_t i = 0; i < 32; i++) {
        uint32_t val = lora_read_reg(this_modem, LORA_REG_RSSI_WIDEBAND);
        delay_nops(10000);
        new_seed = new_seed | ((val & 0x00000001) << i);
    }
#ifdef DEBUG
    fprintf(fp_uart, "generated seed %lx\r\n", new_seed);
#endif
    srand((long)new_seed);
    lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_STDBY);
}
/*
void lora_dbg_print_irq(uint8_t data) {
    if(data & LORA_MASK_IRQFLAGS_RXTIMEOUT)
        fputs("RX timed out\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_RXDONE)
        fputs("RX finished \r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR)
        fputs("CRC error\r\n", fp_uart);

    if(!(data & LORA_MASK_IRQFLAGS_VALIDHEADER))
        fputs("Last header invalid\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_TXDONE)
        fputs("TX complete\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_CADDONE)
        fputs("CAD finished\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_FHSSCHANGECHANNEL)
        fputs("FHSS change channel\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_CADDETECTED)
        fputs("Channel activity detected\r\n", fp_uart);
}
*/
