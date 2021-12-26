/**
 * @file modem_ll_config.h
 *
 * This file provide some data structures and functions that help when working with the LoRa modulation
 * setttings.
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

/**
 * Enum for spreading factor setting
 */
enum spreading_factor_setting { SF6 = 0, SF7, SF8, SF9, SF10, SF11, SF12, SF_MAX };

/**
 * Enum for bandwidth setting
 */
enum bandwidth_setting {
    bandwidth_7800 = 0,
    bandwidth_10400,
    bandwidth_15600,
    bandwidth_20800,
    bandwidth_31250,
    bandwidth_41700,
    bandwidth_62500,
    bandwidth_125000,
    bandwidth_250000,
    bandwidth_500000,
    bandwidth_MAX
};

/**
 * Enum for coding rate
 */
enum coding_rate_setting { CR4_5 = 0, CR4_6, CR4_7, CR4_8, CR_MAX };

/**
 * @struct modulation_config
 *
 * This struct represents a LoRa modulation setting.
 * Payload length is of no consequence if the modem is in implicit header mode.
 */
struct modulation_config {
    enum spreading_factor_setting spreading_factor;
    enum bandwidth_setting bandwidth;
    enum coding_rate_setting coding_rate;
    bool header_enabled;
    bool crc_enabled;
    uint16_t preamble_length;
    uint8_t payload_length;
    uint32_t frequency;
};

/**
 * @brief a helper function for getting the bandwidth value from a bandwidth enum
 *
 * @param bandwith enum representing bandwidth setting
 * @return bandwith in Hz
 */
uint32_t get_bandwidth(enum bandwidth_setting bandwidth);

extern struct modulation_config default_modulation;
extern struct modulation_config long_range_modulation;
extern struct modulation_config short_range_modulation;
