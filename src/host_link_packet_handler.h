

// run host_link modem setup and modulation config first
void host_link_packet_handler_setup(uint8_t *command, uint16_t len);

void host_link_packet_handler_request_transmit(uint8_t *command, uint16_t len);

// persistent or lazy
void host_link_packet_handler_set_tx_mode(uint8_t *command, uint16_t len);

// set max backoffs
void host_link_packet_handler_set_backoffs(uint8_t *command, uint16_t len);