#pragma once

#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

extern uint8_t *cur_command;

void host_link_modem_setup(uint8_t *command, uint16_t len);

void host_link_modem_listen(uint8_t *command, uint16_t len);

void host_link_modem_load_payload(uint8_t *command, uint16_t len);

void host_link_modem_load_and_transmit(uint8_t *command, uint16_t len);

void host_link_modem_transmit(uint8_t *command, uint16_t len);

void host_link_modem_standby(uint8_t *command, uint16_t len);

void host_link_modem_is_clear(uint8_t *command, uint16_t len);

void host_link_modem_get_last_payload_rssi(uint8_t *command, uint16_t len);

void host_link_modem_get_last_payload_snr(uint8_t *command, uint16_t len);

void host_link_modem_get_airtime_usec(uint8_t *command, uint16_t len);

void host_link_modem_read_reg(uint8_t *command, uint16_t len);

void host_link_modem_set_modulation(uint8_t *command, uint16_t len);
