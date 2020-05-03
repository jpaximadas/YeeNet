#pragma once
#include <sys/types.h>
#include <libopencm3/cm3/systick.h>

inline static void delay_nops(uint32_t nops){
	for(uint32_t i = 0; i<nops;i++){
		__asm__("nop");
	}
}

struct pin_port {
	uint32_t port;
	uint32_t pin;
};

extern volatile uint32_t ms;

void timer_setup(void);

static inline uint32_t timer_get_micros(void){
	uint32_t ms_latch = ms;
	uint32_t systick_counter_latch = systick_get_value();
	double micros = systick_counter_latch/71999.0;
	micros = micros * 1000;
	uint32_t retval = (ms_latch*1000) + ((uint32_t)micros);
	return retval;
}

static inline void timer_reset(void){
	ms = 0;
	systick_clear();
}
