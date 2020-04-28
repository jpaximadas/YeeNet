#pragma once

#include <stdbool.h>
#include <sys/types.h>

enum spreading_factor_setting{
    SF6=0,
    SF7,
    SF8,
    SF9,
    SF10,
    SF11,
    SF12
};

enum bandwidth_setting{
    chiprate_7800 = 0,
    chiprate_10400,
    chiprate_15600,
    chiprate_20800,
    chiprate_31250,
    chiprate_41700,
    chiprate_62500,
    chiprate_125000,
    chiprate_250000,
    chiprate_500000
};

enum coding_rate_setting{
    CR4_5=0,
    CR4_6,
    CR4_7,
    CR4_8
};

struct modulation_config{
    enum spreading_factor_setting spreading_factor;
    enum bandwidth_setting bandwidth;
    enum coding_rate_setting coding_rate;
    bool header_enabled;
    bool crc_enabled;
    uint16_t preamble_length;
    uint8_t payload_length ; //this doesn't matter if headers are enabled
};

uint32_t get_chiprate(enum bandwidth_setting chiprate);


extern struct modulation_config default_modulation;
extern struct modulation_config long_range_modulation;
extern struct modulation_config short_range_modulation;
