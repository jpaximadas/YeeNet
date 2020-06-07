
#include <sys/types.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "callback_timer.h"

#define NUM_CALLBACKS 6 //arbitrary number
struct timed_callback callbacks[NUM_CALLBACKS];

#define NUM_TIMERS 6 //arbitrary number
struct timer timers[NUM_TIMERS];

volatile uint32_t timer_ms;

void sys_tick_handler(void){
	timer_ms++;
	
	for(int i = 0; i<NUM_CALLBACKS;i++){
		
		if(timer_ms==0){callbacks[i].wait_rollover = false;} //if a rollover occurs, remove rollover waiting
			
		if(
			!(callbacks[i].is_stale) &&
			callbacks[i].time <= timer_ms &&
			!(callbacks[i].wait_rollover) 
		){
			callbacks[i].is_stale = true;
			(*(callbacks[i].callback_function))(callbacks[i].param);
			
		}
	}
	

}

void callback_timer_setup(void){
	timer_ms = 0;
	for(int i=0;i<NUM_CALLBACKS;i++){
		callbacks[i].is_stale = true;
	}
	for(int i=0;i<NUM_TIMERS;i++){
		timers[i].is_stale = true;
	}
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(71999);
    systick_interrupt_enable();
    systick_counter_enable();
    
    
}

bool add_timed_callback(uint32_t offset, void (*_callback_function)(void *), void *_param, callback_id_t *pos){
	int i=0;
	while(i<NUM_CALLBACKS){
		if(callbacks[i].is_stale){
			
			callbacks[i].time = timer_ms + offset;
			bool wait_rollover = offset<=timer_ms;
			callbacks[i].callback_function = _callback_function;
			callbacks[i].param = _param;
			callbacks[i].is_stale = false;
			*pos = (callback_id_t) i; //report array position
			
			return true;
		}
		i++;
	}
	return false;
}

void remove_timed_callback(callback_id_t pos){
	callbacks[(uint8_t) pos].is_stale = true;
}

//timers mostly for debug purposes
bool start_timer(uint8_t pos){
	if(timers[pos].is_stale){
		timers[pos].start = timer_ms;
		timers[pos].is_stale = false;
		return true;
	}
	return false;
}

uint32_t stop_timer(uint8_t pos){
	timers[ pos].is_stale = true;
	timers[pos].start = timer_ms - timers[pos].start;
	return timers[pos].start;
}