
#include <modem_ll_config.h>

#include <sys/types.h>
#include <stdbool.h>

uint32_t get_chiprate(enum bandwidth_setting chiprate){
	switch(chiprate){
		case chiprate_7800:
			return 7800;
			break;
		case chiprate_10400:
			return 10400;
			break;
		case chiprate_15600:
			return 15600;
			break;
		case chiprate_20800:
			return 20800;
			break;
		case chiprate_31250:
			return 31250;
			break;
		case chiprate_41700:
			return 41700;
			break;
		case chiprate_62500:
			return 62500;
			break;
		case chiprate_125000:
			return 125000;
			break;
		case chiprate_250000:
			return 250000;
			break;
		case chiprate_500000:
			return 500000;
			break;
	}
}

struct modulation_config long_range_modulation = {
    .spreading_factor = SF12,
    .bandwidth = chiprate_500000,
    .coding_rate = CR4_8,
    .header_enabled = true, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 1
};

struct modulation_config short_range_modulation = {
    .spreading_factor = SF6,
    .bandwidth = chiprate_500000,
    .coding_rate = CR4_5,
    .header_enabled = false, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 255
};

struct modulation_config default_modulation = {
    .spreading_factor = SF6,
    .bandwidth = chiprate_500000,
    .coding_rate = CR4_8,
    .header_enabled = false, //enable header
    .crc_enabled = true, //enable crc
    .preamble_length = 8,
    .payload_length = 255
};
