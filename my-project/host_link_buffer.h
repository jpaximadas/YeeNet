#pragma once

#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

void host_link_buffer_pop(uint8_t *command, uint16_t len);

void host_link_buffer_cap(uint8_t *command, uint16_t len);

void host_link_buffer_get_n_overflow(uint8_t *command, uint16_t len);

void host_link_buffer_reset_n_overflow(uint8_t *command, uint16_t len);