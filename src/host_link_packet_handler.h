
#pragma once
#include <sys/types.h>

// run host_link modem setup and modulation config first
void host_link_handler_setup(uint8_t *command, uint16_t len);

/**
 *
 * Command:
 * [0] packet type
 * [1] source: reserved
 * [2] destination
 * [3] length: reserved
 * [4] data length check this
 */
void host_link_handler_request_transmit(uint8_t *command, uint16_t len);

// persistent or lazy
void host_link_handler_set_send_mode(uint8_t *command, uint16_t len);

// set max backoffs
void host_link_handler_set_backoffs_max(uint8_t *command, uint16_t len);

// void host_link_handler_set_carrier_sense(uint8_t *command, uint16_t len);

void host_link_handler_set_timeout(uint8_t *command, uint16_t len);