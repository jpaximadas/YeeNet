/**
 * @file host_link_buffer.h
 *
 * Exposes packet_buffer functionality to the host_link.
 */

#pragma once

#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * Allows the host link to pop an element from the packet buffer if available
 *
 * @param command pointer to command array
 * @param len length of command
 *
 * Postcondition:
 * Response of the follow formating is written to the host_link interface via iface_write:
 * [0] number of packets in buffer when command was executed
 * [1] payload type
 * [2:5] RSSI as int32
 * [6:9] SNR as float
 * [10:13] system time as uint32
 * [14:end] data
 */

void host_link_buffer_pop(uint8_t *command, uint16_t len);
/**
 * Allows the host link to measure the capacity of the buffer
 *
 * @param command pointer to command array
 * @param len length of command
 *
 * Postcondition:
 * Response of the following format is written to the host_link interface via iface_write:
 * [0] number of packets in buffer
 */
void host_link_buffer_cap(uint8_t *command, uint16_t len);

/**
 * Allows the host link to check how many times the packet buffer has overflowed
 *
 * @param command pointer to command array
 * @param len length of command
 *
 * Postcondition:
 * Response of the following format is written to the host link interface via iface_write:
 * [0:3] little-endian integer representing number of overflow events
 */
void host_link_buffer_get_n_overflow(uint8_t *command, uint16_t len);

/**
 * Allows the host link to reset the counter that represents the number of times the packet buffer has
 * overflowed
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_buffer_reset_n_overflow(uint8_t *command, uint16_t len);