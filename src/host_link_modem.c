#include "host_link_modem.h"
#include "modem_hl.h"
#include "modem_ll.h"
#include "modem_ll_config.h"
#include "payload_buffer.h"
#include "platform/platform.h"

struct modem host_link_modem;

void host_link_modem_packet_capture(void *param) {
    struct payload_buffer *buf = (struct payload_buffer *)param;
    struct payload_record *record = payload_record_alloc();  // get a payload record

    if (record != NULL) {
        modem_get_payload(&host_link_modem, record->contents.raw_payload.payload,
                          &(record->contents.raw_payload.len));
        record->rssi = modem_get_last_payload_rssi(&host_link_modem);
        record->snr = modem_get_last_payload_snr(&host_link_modem);
        payload_buffer_push(buf, record);  // push the record to the host link buffer
    }
    modem_listen(&host_link_modem);
}

volatile bool host_link_tx_in_progress = false;
void host_link_modem_tx_done(void *param) {
    (void)param;
    host_link_tx_in_progress = false;
}

uint8_t success;
// TODO add modulation config
void host_link_modem_setup(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    payload_buffer_init(&host_link_payload_buffer);
    success = modem_setup(&host_link_modem, platform_pinout.p_spi, platform_pinout.modem_ss,
                          platform_pinout.modem_rst);
    if (success) {
        link.iface_write_byte(0x00);
    } else {
        link.iface_write_byte(0x01);
    }
    modem_attach_callbacks(&host_link_modem, &host_link_modem_packet_capture, &host_link_modem_tx_done,
                           &host_link_payload_buffer);
    return;
}

void host_link_modem_listen(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    modem_listen(&host_link_modem);
}

void host_link_modem_load_payload(uint8_t *command, uint16_t len) {
    if (len == 0) {
        link.iface_write_byte(0x01);
        return;
    }

    uint16_t max_length;
    if (payload_length_is_fixed(&host_link_modem)) {
        max_length = (host_link_modem.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if (fits) {
        modem_load_payload(&host_link_modem, command, len);
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
    if (payload_length_is_fixed(&host_link_modem)) {
        max_length = (host_link_modem.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if (fits) {
        modem_load_payload(&host_link_modem, command, len);
    } else {
        link.iface_write_byte(0x02);
        return;
    }

    host_link_tx_in_progress = true;
    modem_transmit(&host_link_modem);
    while (host_link_tx_in_progress)
        ;
    link.iface_write_byte(0x00);
    return;
}

void host_link_modem_transmit(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    host_link_tx_in_progress = true;
    modem_transmit(&host_link_modem);
    while (host_link_tx_in_progress)
        ;
    return;
}

void host_link_modem_standby(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    modem_standby(&host_link_modem);
    return;
}

void host_link_modem_is_clear(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    bool status = modem_is_clear(&host_link_modem);
    link.iface_write_byte(status);
    return;
}

void host_link_modem_get_last_payload_rssi(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    int32_t rssi = modem_get_last_payload_rssi(&host_link_modem);
    link.iface_write_bytes((uint8_t *)&rssi, 4);
    return;
}

void host_link_modem_get_last_payload_snr(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    float snr = modem_get_last_payload_snr(&host_link_modem);
    link.iface_write_bytes((uint8_t *)&snr, 4);
    return;
}

void host_link_modem_get_airtime_usec(uint8_t *command, uint16_t len) {
    if (len == 1) {
        link.iface_write_byte(0x00);  // report a good command
        uint32_t airtime_usec = modem_get_airtime_usec(&host_link_modem, command[0]);
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
    struct modulation_config *modulation = host_link_modem.modulation;

    modulation->spreading_factor = (enum spreading_factor_setting)command[0];
    modulation->bandwidth = (enum bandwidth_setting)command[1];
    modulation->coding_rate = (enum coding_rate_setting)command[2];
    modulation->header_enabled = (bool)command[3];
    modulation->crc_enabled = (bool)command[4];
    modulation->preamble_length = *((uint16_t *)(&(command[5])));
    modulation->payload_length = command[7];
    modulation->frequency = frequency;

    lora_change_mode(&host_link_modem, SLEEP);
    lora_config_modulation(&host_link_modem);
    lora_change_mode(&host_link_modem, STANDBY);
    link.iface_write_byte(0x00);  // report a good command
    return;
}