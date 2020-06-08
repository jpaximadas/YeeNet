#pragma once
#include <sys/types.h>
#include <libopencm3/cm3/systick.h>

//these are supposed to be immutable
typedef uint8_t callback_id_t;
typedef uint8_t timer_id_t;

extern volatile uint32_t timer_ms;

static inline uint32_t get_timer_val(void){
	return timer_ms;
}

struct timed_callback{
	bool is_stale;
	uint32_t time;
	bool wait_rollover;
	void (*callback_function)(void *);
	void *param;
};

struct timer{
	bool is_stale;
	uint32_t start;
};

void callback_timer_setup(void);

void remove_timed_callback(callback_id_t pos);
	
bool add_timed_callback(uint32_t offset, void (*_callback_function)(void *), void *_param, callback_id_t *pos);

bool start_timer(uint8_t pos);

uint32_t stop_timer(uint8_t pos);