#include "callback_timer.h"  //debug
#include "modem_hl.h"
#include "modem_ll.h"
#include "modem_ll_config.h"
#include "uart.h"
#include "util.h"
#include <libopencm3/stm32/gpio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof(*(x)))

#define FXOSC 32000000
#define FREQ_TO_REG(in_freq) ((uint32_t)((((uint64_t)in_freq) << 19) / FXOSC))
#define REG_TO_FREQ(in_reg) ((uint32_t)((FXOSC * in_reg) >> 19))

static void dummy_callback(void *param);

static void dummy_callback(void *param) {
    return;
}

bool modem_setup(struct modem *this_modem,
                 uint32_t spi_interface,
                 pin_descriptor_t ss,
                 pin_descriptor_t rst) {

    // set up irq info in struct
    this_modem->irq_data = 0;
    this_modem->irq_seen = true;

    // store hardware data
    this_modem->rst = rst;
    this_modem->spi_interface = spi_interface;
    this_modem->ss = ss;

    // set rx/tx done isr to dummy functions
    this_modem->rx_callback = &dummy_callback;
    this_modem->tx_callback = &dummy_callback;

    //drive ss high
    ss_set(this_modem);

    // TODO: Allow user-configurable interrupt pins
    // For now, assume exti0 is used on A0
    exti0_modem = this_modem;

    // Reset the board
    gpio_clear(this_modem->rst.port, this_modem->rst.pin);
    delay_nops(1000000);
    gpio_set(this_modem->rst.port, this_modem->rst.pin);

    // assume max power settings
    // Configure PA_BOOST with max power
    lora_write_reg_and_check(this_modem, LORA_REG_PA_CONFIG, 0x8f, true);
    // Enable overload current protection with max trim
    lora_write_reg_and_check(this_modem, LORA_REG_OCP, 0x3f, true);
    // Set RegPaDac to 0x87, increases power?
    lora_write_reg_and_check(this_modem, REG_PA_DAC, 0x84, true);

    // set sync word
    // lora_write_reg(this_modem,0x39,0x00);

    // Set the FIFO RX/TX pointers to 0
    lora_write_reg(this_modem, LORA_REG_FIFO_TX_BASE_ADDR, 0x00);
    lora_write_reg(this_modem, LORA_REG_FIFO_RX_BASE_ADDR, 0x00);

    // Disable frequency hopping
    lora_write_reg(this_modem, LORA_REG_HOP_PERIOD, 0x00);

    // Set frequency
    /*
    lora_write_reg(this_modem,LORA_REG_FR_MSB, (FREQ_TO_REG(915000000) >> 16) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_MID, (FREQ_TO_REG(915000000) >> 8) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_LSB, FREQ_TO_REG(915000000) & 0b11111111);
    */
    lora_write_reg(this_modem, LORA_REG_FR_MSB, (FREQ_TO_REG(920000000) >> 16) & 0b11111111);
    lora_write_reg(this_modem, LORA_REG_FR_MID, (FREQ_TO_REG(920000000) >> 8) & 0b11111111);
    lora_write_reg(this_modem, LORA_REG_FR_LSB, FREQ_TO_REG(920000000) & 0b11111111);
    // fprintf(fp_uart,"starting modulation config\r\n");
    lora_config_modulation(this_modem, &default_modulation);
    switch (this_modem->modulation->spreading_factor) {
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
            this_modem->extra_time_ms = 15;
            break;
    }
    seed_random(this_modem);

    return true;
}

void modem_attach_callbacks(struct modem *this_modem,
                            void (*_rx_callback)(void *),
                            void (*_tx_callback)(void *),
                            void *_callback_arg) {
    // set rx/tx done isr function pointers
    this_modem->callback_arg = _callback_arg;
    this_modem->rx_callback = _rx_callback;
    this_modem->tx_callback = _tx_callback;
}

// this function will put the lora into a standby mode
void modem_load_payload(struct modem *this_modem, uint8_t msg[MAX_PAYLOAD_LENGTH], uint8_t length) {

    lora_change_mode(this_modem, STANDBY, false);

    if (this_modem->modulation->header_enabled) {                     // explicit header mode
        lora_write_reg(this_modem, LORA_REG_PAYLOAD_LENGTH, length);  // inform lora of payload length
        lora_write_fifo(this_modem, msg, length, 0);
    } else {  // fixed length, implicit mode

        if (length > this_modem->modulation->payload_length) {
            length = this_modem->modulation->payload_length;
        }

        lora_write_fifo(this_modem, msg, length, 0);
    }
}

void modem_transmit(struct modem *this_modem) {
    this_modem->irq_seen = true;
    lora_change_mode(this_modem, TX, false);
}

void modem_load_and_transmit(struct modem *this_modem, uint8_t msg[MAX_PAYLOAD_LENGTH], uint8_t length) {
    modem_load_payload(this_modem, msg, length);
    modem_transmit(this_modem);
}

bool modem_listen(struct modem *this_modem) {
    this_modem->irq_seen = true;  // reset irq flag
    return lora_change_mode(this_modem, RX, false);
}

// this function will put the lora into a standby mode
enum payload_status modem_get_payload(struct modem *this_modem,
                                      uint8_t buf_out[MAX_PAYLOAD_LENGTH],
                                      uint8_t *length) {
    uint8_t data = this_modem->irq_data;
    if (this_modem->irq_seen || (this_modem->cur_irq_type != RX_DONE)) {
        return PAYLOAD_EMPTY;
    }
    lora_change_mode(this_modem, STANDBY, false);  // put the lora into standby mode for reg read
    enum payload_status retval = PAYLOAD_GOOD;

    if (!(data & LORA_MASK_IRQFLAGS_RXDONE)) {
        retval = PAYLOAD_EMPTY;
    }

    if (data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR) {
        retval = PAYLOAD_BAD;
    }

    if (this_modem->modulation->header_enabled) {  // explicit header mode, variables length
        *length = lora_read_reg(this_modem, LORA_REG_RX_NB_BYTES);
    } else {  // implicit header mode, fixed length
        *length = this_modem->modulation->payload_length;
    }

    // FIFO contains valid packet, return it
    uint8_t ptr = lora_read_reg(this_modem, LORA_REG_FIFO_RX_CUR_ADDR);
    // lora_write_reg(this_modem, LORA_REG_OP_MODE,MODE_LORA | MODE_STDBY);
    lora_read_fifo(this_modem, buf_out, *length, ptr);
    return retval;
}

double ceil(double num);

double ceil(double num) {
    int32_t inum = (int32_t)num;  // cast to floor
    if (num > (double)inum) {     // if floored number is lower than input, add 1
        inum++;
    }
    // fprintf(fp_uart,"floored to %lu\r\n",inum);
    return (double)inum;
}

uint32_t modem_get_airtime_usec(struct modem *this_modem, uint8_t payload_length) {
    // fprintf(fp_uart,"-----------------------\r\n");
    // fprintf(fp_uart,"airtime computation in progress...\r\n");
    int32_t payload_bytes = (int32_t)payload_length;
    int32_t implicit_header = 0;  // implicit header = 0 when header is enabled
    if (!(this_modem->modulation->header_enabled)) {
        // header isn't enabled
        // implicit header mode
        // IH=1
        implicit_header = 1;
        payload_bytes = this_modem->modulation->payload_length;
    }

    int32_t spreading_factor = (this_modem->modulation->spreading_factor) + 6;

    int32_t SF_pw = 0x1 << (spreading_factor);
    int32_t BW = get_bandwidth(this_modem->modulation->bandwidth);
    double symbol_time = ((double)SF_pw / (double)BW);
    int32_t symbol_time_usec = (int32_t)(symbol_time * 1E6);  // symbol length in us

    // fprintf(fp_uart,"symbol time usec %lu\r\n",symbol_time_usec);

    int32_t low_data_rate_optimize;
    // test ? true : false
    if (symbol_time_usec > 16000) {
        low_data_rate_optimize = 1;
        // fprintf(fp_uart,"LOW DATA RATE OPTIMIZE ENABLED\r\n");
    } else {
        low_data_rate_optimize = 0;
    }

    int32_t crc;
    // test ? true : false
    (this_modem->modulation->crc_enabled) ? (crc = 1) : (crc = 0);
    // fprintf

    int32_t coding_rate = ((int32_t)this_modem->modulation->coding_rate) + 1;

    int32_t n_preamble = this_modem->modulation->preamble_length;  // number of preamble symbols

    /*
    fprintf(fp_uart,"payload bytes: %lu\r\n",payload_bytes);
    fprintf(fp_uart,"spreading factor: %lu\r\n",spreading_factor);
    fprintf(fp_uart,"crc: %lu\r\n",crc);
    fprintf(fp_uart,"implicit header: %lu\r\n",implicit_header);
    fprintf(fp_uart,"LDRO: %lu\r\n",low_data_rate_optimize);
    fprintf(fp_uart,"coding rate: %lu\r\n",coding_rate);
    */

    // just an intermediate step
    double n_payload =
        ((double)(8 * payload_bytes - 4 * spreading_factor + 28 + 16 * crc - 20 * implicit_header)) /
        ((double)(4 * (spreading_factor - 2 * low_data_rate_optimize)));

    n_payload = ceil(n_payload) * (((double)coding_rate) + 4.0);

    if (n_payload < 0.0) {
        n_payload = 0.0;
    }

    n_payload = n_payload + 8.0;
    // fprintf(fp_uart,"symbols in payload:%lu\r\n",(uint32_t)(n_payload));

    double payload_time = n_payload * symbol_time;

    double preamble_time = ((double)n_preamble + 4.25F) * symbol_time;

    double packet_time = payload_time + preamble_time;
    // fprintf(fp_uart,"-----------------------\r\n");
    return ((uint32_t)(packet_time * 1E6));
}

bool modem_is_clear(struct modem *this_modem) {
    uint8_t reg = lora_read_reg(this_modem, LORA_REG_MODEM_STAT);
    // fprintf(fp_uart,"modem stat reg:0x%x\r\n",reg);
    if ((reg & 0x3) != 0x0) {  // ensure signal synced, and signal detected are cleared
        return false;
    }
    if ((lora_read_reg(this_modem, LORA_REG_OP_MODE) & 0x7) == 0x3) {
        return false;
    }
    return true;
}

int32_t get_last_payload_rssi(struct modem *this_modem) {
    int32_t reg = lora_read_reg(this_modem, LORA_REG_PKT_RSSI_VALUE);
    return -157 + reg;  // assume use of HF port see page 112 of lora manual
}

double get_last_payload_snr(struct modem *this_modem) {
    int32_t reg = lora_read_reg(this_modem, LORA_REG_PKT_SNR_VALUE);
    double snr = ((double)reg) / 4.0;
    if (reg >= 0) {
        return snr;
    } else {
        return ((double)get_last_payload_rssi(this_modem)) + snr;
    }
}

bool payload_length_is_fixed(struct modem *this_modem) {
    return !(this_modem->modulation->header_enabled);
}
