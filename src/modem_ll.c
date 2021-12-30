#include "callback_timer.h"
#include "modem_ll.h"
#include "modem_ll_config.h"
#include "platform/platform.h"
#include "sx127x.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdbool.h>
#include <sys/types.h>

// function prototypes

struct modem *volatile exti0_modem = NULL;

void exti0_isr(void) {
    // TODO figure out how to demote systick priority
    exti_reset_request(EXTI0);  // must always be first
    exti0_modem->last_rx_time = system_ms;
    exti0_modem->irq_data = lora_read_reg(exti0_modem, LORA_REG_IRQFLAGS);

    // irq flag register must be reset twice. Is this hardware errata?
    lora_write_reg(exti0_modem, LORA_REG_IRQFLAGS, 0xFF);
    lora_write_reg(exti0_modem, LORA_REG_IRQFLAGS, 0xFF);

    exti0_modem->irq_seen = false;

    switch (exti0_modem->cur_irq_type) {
        case RX_DONE:
            (*(exti0_modem->rx_callback))(exti0_modem->callback_arg);
            break;
        case TX_DONE:
            platform_set_indicator(false);
            (*(exti0_modem->tx_callback))(exti0_modem->callback_arg);
            break;
        case INVALID:
            break;
    }

    // systick_counter_enable();
}

void lora_write_fifo(struct modem *this_modem,
                     uint8_t *buf,
                     uint8_t len,
                     uint8_t trailing_zeros,
                     uint8_t offset) {
    // clip the number of trailing zeros if there are too many
    uint8_t max_zeros = MAX_PAYLOAD_LENGTH - len;
    if (trailing_zeros > max_zeros) {
        trailing_zeros = max_zeros;
    }

    // Update modem's FIFO address pointer
    ss_clear(this_modem);
    spi_xfer(this_modem->spi_interface, LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(this_modem->spi_interface, offset);
    ss_set(this_modem);

    delay_nops(1000);

    // Write data to FIFO.
    ss_clear(this_modem);
    spi_xfer(this_modem->spi_interface, LORA_REG_FIFO | WRITE_MASK);
    for (uint8_t i = 0; i < len; i++) {
        spi_xfer(this_modem->spi_interface, buf[i]);
    }
    for (uint8_t i = 0; i < trailing_zeros; i++) {
        spi_xfer(this_modem->spi_interface, 0);
    }
    ss_set(this_modem);
}

void lora_read_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset) {
    // Update modem's FIFO address pointer
    ss_clear(this_modem);
    spi_xfer(this_modem->spi_interface, LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(this_modem->spi_interface, offset);
    ss_set(this_modem);

    delay_nops(1000);
    // Read data from FIFO
    ss_clear(this_modem);
    spi_xfer(this_modem->spi_interface, LORA_REG_FIFO);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_xfer(this_modem->spi_interface, 0x00);
    }
    ss_set(this_modem);
}

void lora_change_mode(struct modem *this_modem, enum lora_mode change_to) {
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
            platform_set_indicator(true);
            break;
        default:
            mode = SLEEP;
            break;
    }
    lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | mode);
    return;
}

uint8_t lora_read_reg(struct modem *this_modem, uint8_t reg) {
    // gpio_set(GPIOC,GPIO13);
    // fprintf(fp_uart,"reading from reg: %x\r\n",reg);

    ss_clear(this_modem);
    // gpio_clear(GPIOC,GPIO13);

    spi_xfer(this_modem->spi_interface, reg & 0x7F);
    uint8_t ret = spi_xfer(this_modem->spi_interface, 0);

    ss_set(this_modem);

    return ret;
}

void lora_write_reg(struct modem *this_modem, uint8_t reg, uint8_t val) {
    // fprintf(fp_uart,"writing to reg: %x\r\n",reg);

    ss_clear(this_modem);

    spi_xfer(this_modem->spi_interface, reg | WRITE_MASK);
    spi_xfer(this_modem->spi_interface, val);

    ss_set(this_modem);
}

#define FXOSC 32000000
#define FREQ_TO_REG(in_freq) ((uint32_t)((((uint64_t)in_freq) << 19) / FXOSC))
#define REG_TO_FREQ(in_reg) ((uint32_t)((FXOSC * in_reg) >> 19))

void lora_config_modulation(struct modem *this_modem) {
    // get the modem modulation
    struct modulation_config *modulation = this_modem->modulation;

    // program extra time
    switch (modulation->spreading_factor) {
        case SF6:
            this_modem->extra_time_ms = 5;
            break;
        case SF7:
            this_modem->extra_time_ms = 1;
            break;
        case SF8:
            this_modem->extra_time_ms = 1;
            break;
        case SF9:
            this_modem->extra_time_ms = 1;
            break;
        case SF10:
            this_modem->extra_time_ms = 3;
            break;
        case SF11:
            this_modem->extra_time_ms = 6;
            break;
        case SF12:
        case SF_MAX:
            this_modem->extra_time_ms = 15;
            break;
    }

    // set the frequency
    lora_write_reg(this_modem, LORA_REG_FR_MSB, (FREQ_TO_REG(modulation->frequency) >> 16) & 0b11111111);
    lora_write_reg(this_modem, LORA_REG_FR_MID, (FREQ_TO_REG(modulation->frequency) >> 8) & 0b11111111);
    lora_write_reg(this_modem, LORA_REG_FR_LSB, FREQ_TO_REG(modulation->frequency) & 0b11111111);

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
}

void seed_random(struct modem *this_modem) {
    uint32_t new_seed = 0;
    lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_RXCON);
    for (uint8_t i = 0; i < 32; i++) {
        uint32_t val = lora_read_reg(this_modem, LORA_REG_RSSI_WIDEBAND);
        delay_nops(10000);
        new_seed = new_seed | ((val & 0x00000001) << i);
    }
    srand((long)new_seed);
    lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_STDBY);
}