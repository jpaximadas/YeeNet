/**
 * @file host_link_modem.h
 * @brief provides the host link jump table with modem access
 *
 * Each function in this file wraps a modem_hl functionality.
 * This wrapping is nearly one-to-one but there is some additional functionality needed to make this layer
 * useful such as packet buffering. The arguments to a function are standard to all function in the host link
 * jump table. Each function also has its own format for incoming commands and responses.
 *
 * TODO: add option to return to listening upon packet reception or remain in a standby mode
 */

#pragma once

#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * @brief modem_setup wrapper for the host_link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * None
 *
 * @param command pointer to command array (unused)
 * @param len length of command (unused)
 */
void host_link_modem_setup(uint8_t *command, uint16_t len);

/**
 * @brief modem_listen wrapper for the host_link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * None
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_listen(uint8_t *command, uint16_t len);

/**
 * @brief modem_load_payload wrapper for the host_link jump table
 *
 * Command format:
 * [0:end] payload bytes
 *
 * Response format:
 * [0] 0x00 if OK, 0x01 if empty command, 0x02 if too long
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_load_payload(uint8_t *command, uint16_t len);

/**
 * @brief modem_load_and_transmit wrapper for the host_link jump table
 *
 * This function will not return until TX procedure is complete.
 * Keep in mind TX may take upwards for 1 second depending on modulation settings and packet size.
 *
 * Command format:
 * [0:end] payload bytes
 *
 * Response format
 * [0] 0x00 if OK, 0x01 if empty command, 0x02 if too long
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_load_and_transmit(uint8_t *command, uint16_t len);

/**
 * @brief modem_transmit wrapper for the host_link jump table
 *
 * This function will not return until TX procedure is complete.
 *
 * Command format:
 * None
 *
 * Response format
 * None
 *
 *  * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_transmit(uint8_t *command, uint16_t len);

/**
 * @brief modem_standby wrapper for the host_link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * None
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_standby(uint8_t *command, uint16_t len);

/**
 * @brief modem_is_clear wrapper for the host link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * [0] truth-y if modem is clear, false-y if not
 *
 * @param command pointer to command array
 * @param len length of command
 */
void host_link_modem_is_clear(uint8_t *command, uint16_t len);

/**
 * @brief modem_get_last_payload_rssi wrapper for the host link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * [0:3] little-endian 32 bit two's complement integer
 */
void host_link_modem_get_last_payload_rssi(uint8_t *command, uint16_t len);

/**
 * @brief modem_get_last_payload_snr wrapper for the host link jump table
 *
 * Command format:
 * None
 *
 * Response format:
 * [0:3] 32-bit floating point number
 */
void host_link_modem_get_last_payload_snr(uint8_t *command, uint16_t len);

/**
 * @brief modem_get_airtime_usec wrapper for the host link jump table
 *
 * Command format:
 * [0] length of hypothetical packet
 *
 * Response format:
 * [0] 0x00 if OK, 0x01 if length of packet isn't present or command is too long
 * [1:4] little-endian unsigned 32 bit integer representing airtime for a packet in microseconds
 */
void host_link_modem_get_airtime_usec(uint8_t *command, uint16_t len);

/**
 * @brief configures modulation settings of the modem
 *
 * Note that this does not wrap modem_hl functionality.
 * This function wraps modem_ll functionality specific to the SX1276 LoRa mode.
 * This abstraction could be improved.
 *
 * Command format:
 * [0] SF
 * [1] bandwidth setting, corresponds to bandwidth_setting enum
 * [2] coding rate setting, corresponds to coding_rate enum
 * [3] CRC enabled, boolean value
 * [4] header enabled, boolean value
 * [5-6] preamble length, unsigned little-endian 16 bit integer
 * [7] payload length, unsigned 8 bit integer relevant only in explicit header mode
 * [8-11] frequency, unsigned little-endian 32 bit integer representing frequency in Hz
 *
 * Response format:
 * [0] 0x00 means OK, 0x01 means the command had invalid values
 */
void host_link_modem_set_modulation(uint8_t *command, uint16_t len);

// TODO: Add get modulation function