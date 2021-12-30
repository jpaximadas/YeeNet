#include "host_link_modem.h"
#include "modem_hl.h"
#include "modem_ll.h"
#include "modem_ll_config.h"
#include "payload_buffer.h"
#include "platform/platform.h"

void host_link_modem_packet_capture(void *param) {
    struct payload_buffer *buf = (struct payload_buffer *)param;
    struct payload_record *record = payload_record_alloc();  // get a payload record

    if (record != NULL) {
        enum payload_status stat = modem_get_payload(&modem0, record->contents.raw, &(record->len));
        record->rssi = modem_get_last_payload_rssi(&modem0);
        record->snr = modem_get_last_payload_snr(&modem0);
        record->time = modem_get_last_rx_time(&modem0);
        switch (stat) {
            case PAYLOAD_GOOD:
                record->type = RAW_GOOD;
                payload_buffer_push(buf, record);  // push the record to the host link buffer
                break;
            case PAYLOAD_BAD:
                record->type = RAW_BAD;
                payload_buffer_push(buf, record);
                break;
            case PAYLOAD_EMPTY:
            default:
                payload_record_free(record);
                break;
        }
    }
    modem_listen(&modem0);
}

volatile bool host_link_tx_in_progress = false;
void host_link_modem_tx_done(void *param) {
    (void)param;
    host_link_tx_in_progress = false;
}

// TODO add modulation config
void host_link_modem_setup(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    payload_buffer_init(&host_link_payload_buffer);
    modem_setup(&modem0, platform_pinout.p_spi, platform_pinout.modem_ss, platform_pinout.modem_rst);
    modem_attach_callbacks(&modem0, &host_link_modem_packet_capture, &host_link_modem_tx_done,
                           &host_link_payload_buffer);
    return;
}

void host_link_modem_listen(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    modem_listen(&modem0);
}

void host_link_modem_load_payload(uint8_t *command, uint16_t len) {
    if (len == 0) {
        link.iface_write_byte(0x01);
        return;
    }

    uint16_t max_length;
    if (payload_length_is_fixed(&modem0)) {
        max_length = (modem0.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if (fits) {
        modem_load_payload(&modem0, command, len);
    } else {
        link.iface_write_byte(0x02);
        return;
    }

    link.iface_write_byte(0x00);
    return;
}

/**
 *  0 is OK, 1 is empty packet, 2 is too long
 */
void host_link_modem_load_and_transmit(uint8_t *command, uint16_t len) {
    if (len == 0) {
        link.iface_write_byte(0x01);
        return;
    }

    uint16_t max_length;
    if (payload_length_is_fixed(&modem0)) {
        max_length = (modem0.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if (fits) {
        modem_load_payload(&modem0, command, len);
    } else {
        link.iface_write_byte(0x02);
        return;
    }

    host_link_tx_in_progress = true;
    modem_transmit(&modem0);
    while (host_link_tx_in_progress)
        ;
    link.iface_write_byte(0x00);
    return;
}

void host_link_modem_transmit(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    host_link_tx_in_progress = true;
    modem_transmit(&modem0);
    while (host_link_tx_in_progress)
        ;
    return;
}

void host_link_modem_standby(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    modem_standby(&modem0);
    return;
}

void host_link_modem_is_clear(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    bool status = modem_is_clear(&modem0);
    link.iface_write_byte(status);
    return;
}

void host_link_modem_get_last_payload_rssi(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    int32_t rssi = modem_get_last_payload_rssi(&modem0);
    link.iface_write_bytes((uint8_t *)&rssi, 4);
    return;
}

void host_link_modem_get_last_payload_snr(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    float snr = modem_get_last_payload_snr(&modem0);
    link.iface_write_bytes((uint8_t *)&snr, 4);
    return;
}

void host_link_modem_get_airtime_usec(uint8_t *command, uint16_t len) {
    if (len == 1) {
        link.iface_write_byte(0x00);  // report a good command
        uint32_t airtime_usec = modem_get_airtime_usec(&modem0, command[0]);
        link.iface_write_bytes((uint8_t *)&airtime_usec, 4);
        return;
    } else {
        link.iface_write_byte(0x01);  // report a bad command
    }
}

void host_link_modem_set_modulation(uint8_t *command, uint16_t len) {
    if (len != 12) {
        link.iface_write_byte(0x01);  // report a bad command
        return;
    }
    if (command[0] >= SF_MAX) {
        link.iface_write_byte(0x01);
        return;
    }
    if (command[1] >= bandwidth_MAX) {
        link.iface_write_byte(0x01);
        return;
    }
    if (command[2] >= CR_MAX) {
        link.iface_write_byte(0x01);
        return;
    }
    volatile uint32_t frequency = *((uint32_t *)(&(command[8])));
    // check if in ISM band
    if ((frequency > 928000000) || (frequency < 902000000)) {
        link.iface_write_byte(0x01);
        return;
    }
    struct modulation_config *modulation = modem0.modulation;

    modulation->spreading_factor = (enum spreading_factor_setting)command[0];
    modulation->bandwidth = (enum bandwidth_setting)command[1];
    modulation->coding_rate = (enum coding_rate_setting)command[2];
    modulation->header_enabled = (bool)command[3];
    modulation->crc_enabled = (bool)command[4];
    modulation->preamble_length = *((uint16_t *)(&(command[5])));
    modulation->payload_length = command[7];
    modulation->frequency = frequency;

    lora_change_mode(&modem0, SLEEP);
    lora_config_modulation(&modem0);
    lora_change_mode(&modem0, STANDBY);
    link.iface_write_byte(0x00);  // report a good command
    return;
}