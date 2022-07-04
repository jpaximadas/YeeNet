#include "host_link.h"
#include "host_link_packet_handler.h"
#include "packet_handler.h"
#include "payload_buffer.h"

bool host_link_handler_pkt_rdy_callback(void *param);

bool host_link_handler_pkt_rdy_callback(void *param) {
    struct payload_buffer *buf = (struct payload_buffer *)param;
    struct payload_record *rec = handler0.rx_pkt;
    rec->rssi = modem_get_last_payload_rssi(&modem0);
    rec->snr = modem_get_last_payload_snr(&modem0);
    rec->type = PACKET;
    rec->time = modem_get_last_rx_time(handler0.my_modem);
    payload_buffer_push(&host_link_payload_buffer, rec);
    rec = payload_record_alloc();
    if (rec == NULL) {
        return false;
    } else {
        handler_set_rx_pkt_pointer(&handler0, rec);
        return true;
    }
}

// run host_link modem setup and modulation config first
void host_link_handler_setup(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    struct payload_record *rec = payload_record_alloc();
    handler_setup(&handler0, &modem0, rec, &host_link_handler_pkt_rdy_callback, &host_link_payload_buffer,
                  PERSISTENT, 8);
}

uint32_t timeout_ms = 100;
void host_link_handler_request_transmit(uint8_t *command, uint16_t len) {
    if (len > MAX_PAYLOAD_LENGTH) {
        link.iface_write_byte(0x01);  // packet is too long
        return;
    }
    if (len < PACKET_DATA_OVERHEAD) {
        link.iface_write_byte(0x02);  // packet is too short
    }
    struct packet_data *pkt = (struct packet_data *)command;

    // populate source and length
    pkt->src = local_address_get();
    pkt->len = len - PACKET_DATA_OVERHEAD;

    bool ret = false;
    uint32_t start = system_ms;
    while (!ret && ((start + timeout_ms) > system_ms)) {
        ret = handler_request_transmit(&handler0, pkt);
    }
    if (!ret) {
        link.iface_write_byte(0x03);  // transmit failed due to timeout
        return;
    }

    while (handler0.my_state == LOCKED)
        ;  // wait for transmission to be complete
    if (handler0.last_packet_status == SUCCESS) {
        link.iface_write_byte(0x00);  // packet sent
    } else {
        link.iface_write_byte(0x04);  // handler reported failure
    }
}

// persistent or lazy
void host_link_handler_set_send_mode(uint8_t *command, uint16_t len) {
    if (len != 1) {
        link.iface_write_byte(0x01);  // malformed command
        return;
    }
    if (command[0] != 0 || command[0] != 1) {
        link.iface_write_byte(0x01);  // wrong frist byte
        return;
    }
    handler0.my_send_mode = (enum send_mode)command[0];
    link.iface_write_byte(0x00);  // success
}

// set max backoffs
void host_link_handler_set_backoffs_max(uint8_t *command, uint16_t len) {
    if (len != 1) {
        link.iface_write_byte(0x01);
        return;
    }
    handler0.backoffs_max = command[0];
    link.iface_write_byte(0x00);
    return;
}

// void host_link_handler_set_carrier_sense(uint8_t *command, uint16_t len);

void host_link_handler_set_timeout(uint8_t *command, uint16_t len) {
    if (len != 4) {
        link.iface_write_byte(0x01);
        return;
    }
    for (uint8_t i = 0; i < 4; i++) {
        ((uint8_t *)timeout_ms)[i] = command[i];
    }
    link.iface_write_byte(0x00);
    return;
}

void host_link_handler_set_sense(uint8_t *command, uint16_t len) {
    if (len != 1) {
        link.iface_write_byte(0x01);  // command too long
        return;
    }
    handler_set_sense_mode(&handler0, command[0]);
    link.iface_write_byte(0x00);  // success
}