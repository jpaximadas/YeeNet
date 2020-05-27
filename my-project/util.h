#pragma once
#include <sys/types.h>

inline static void delay_nops(uint32_t nops){
	for(uint32_t i = 0; i<nops;i++){
		__asm__("nop");
	}
}

struct pin_port {
	uint32_t port;
	uint32_t pin;
};
