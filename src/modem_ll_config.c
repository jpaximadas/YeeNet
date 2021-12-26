
#include <modem_ll_config.h>
#include <stdbool.h>
#include <sys/types.h>

uint32_t get_bandwidth(enum bandwidth_setting bandwidth) {
    uint32_t retval = 7800;
    switch (bandwidth) {
        case bandwidth_7800:
            retval = 7800;
            break;
        case bandwidth_10400:
            retval = 10400;
            break;
        case bandwidth_15600:
            retval = 15600;
            break;
        case bandwidth_20800:
            retval = 20800;
            break;
        case bandwidth_31250:
            retval = 31250;
            break;
        case bandwidth_41700:
            retval = 41700;
            break;
        case bandwidth_62500:
            retval = 62500;
            break;
        case bandwidth_125000:
            retval = 125000;
            break;
        case bandwidth_250000:
            retval = 250000;
            break;
        case bandwidth_500000:
        case bandwidth_MAX:
            retval = 500000;
            break;
    }
    return retval;
}

struct modulation_config long_range_modulation = {.spreading_factor = SF12,
                                                  .bandwidth = bandwidth_500000,
                                                  .coding_rate = CR4_8,
                                                  .header_enabled = true,  // enable header
                                                  .crc_enabled = true,     // enable crc
                                                  .preamble_length = 8,
                                                  .payload_length = 1,
                                                  .frequency = 920000000};

struct modulation_config short_range_modulation = {.spreading_factor = SF6,
                                                   .bandwidth = bandwidth_500000,
                                                   .coding_rate = CR4_5,
                                                   .header_enabled = false,  // enable header
                                                   .crc_enabled = true,      // enable crc
                                                   .preamble_length = 8,
                                                   .payload_length = 255,
                                                   .frequency = 920000000};

struct modulation_config default_modulation = {.spreading_factor = SF6,
                                               .bandwidth = bandwidth_500000,
                                               .coding_rate = CR4_8,
                                               .header_enabled = false,  // enable header
                                               .crc_enabled = true,      // enable crc
                                               .preamble_length = 8,
                                               .payload_length = 255,
                                               .frequency = 920000000};
