
#include <modem_ll_config.h>

#include <sys/types.h>
#include <stdbool.h>

uint32_t get_bandwidth(enum bandwidth_setting bandwidth){
	switch(bandwidth){
		case bandwidth_7800:
			return 7800;
			break;
		case bandwidth_10400:
			return 10400;
			break;
		case bandwidth_15600:
			return 15600;
			break;
		case bandwidth_20800:
			return 20800;
			break;
		case bandwidth_31250:
			return 31250;
			break;
		case bandwidth_41700:
			return 41700;
			break;
		case bandwidth_62500:
			return 62500;
			break;
		case bandwidth_125000:
			return 125000;
			break;
		case bandwidth_250000:
			return 250000;
			break;
		case bandwidth_500000:
			return 500000;
			break;
	}
}

struct modulation_config long_range_modulation = {
    .spreading_factor = SF12,
    .bandwidth = bandwidth_500000,
    .coding_rate = CR4_8,
    .header_enabled = true, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 1
};

struct modulation_config short_range_modulation = {
    .spreading_factor = SF6,
    .bandwidth = bandwidth_500000,
    .coding_rate = CR4_5,
    .header_enabled = false, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 255
};

struct modulation_config default_modulation = {
    .spreading_factor = SF7,
    .bandwidth = bandwidth_500000,
    .coding_rate = CR4_8,
    .header_enabled = true, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 255
};
