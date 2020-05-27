#pragma once
#include <sys/types.h>
#include <libopencm3/cm3/systick.h>

//these are supposed to be immutable
typedef uint8_t callback_id_t ;

struct timed_callback{
	bool is_stale;
	uint32_t time;
	bool wait_rollover;
	void (*callback_function)(void *);
	void *param;
};

void callback_timer_setup(void);

void remove_timed_callback(callback_id_t pos);
	
bool add_timed_callback(uint32_t offset, void (*_callback_function)(void *), void *_param, callback_id_t *pos);
