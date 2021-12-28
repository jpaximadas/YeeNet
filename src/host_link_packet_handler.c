
#include "packet_handler.h"
#include "payload_buffer.h"

void host_link_handler_pkt_rdy_callback(void *param);

void host_lnk_handler_pkt_rdy_callback(void *param) {
    struct payload_record *rec = (struct payload_buffer *)param;
    payload_buffer_push(&host_link_payload_buffer, rec);
    rec = payload_record_alloc();
    handler_set_rx_pkt_pointer(rec
    // push to the buffer and
}

// run host_link modem setup and modulation config first
void host_link_handler_setup(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    struct payload_record *rec = payload_buffer_alloc();
    handler_setup(&handler0,&modem0,&host_link_handler_pkt_rdy_callback,?,?,PERSISTENT,8);
}

void host_link_handler_request_transmit(uint8_t *command, uint16_t len);

// persistent or lazy
void host_link_handler_set_tx_mode(uint8_t *command, uint16_t len);

// set max backoffs
void host_link_handler_set_backoffs(uint8_t *command, uint16_t len);