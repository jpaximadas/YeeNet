
#include <sys/types.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "callback_timer.h"

#define NUM_CALLBACKS 6 //arbitrary number
struct timed_callback callbacks[NUM_CALLBACKS];

volatile uint32_t timer_ms;

void sys_tick_handler(void){
	timer_ms++;
	
	for(int i = 0; i<NUM_CALLBACKS;i++){
		struct timed_callback *cur_callback = &callbacks[i];
		
		if(timer_ms==0){cur_callback->wait_rollover = false;} //if a rollover occurs, remove rollover waiting
			
		if(
			!(cur_callback->wait_rollover) &&
			!(cur_callback->is_stale) &&
			cur_callback->time <= timer_ms
		){
			cur_callback->is_stale = true;
			(*(cur_callback->callback_function))(cur_callback->param);
			
		}
	}
	

}

void callback_timer_setup(void){
	timer_ms = 0;
	for(int i=0;i<NUM_CALLBACKS;i++){
		callbacks[i].is_stale = true;
	}
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(71999);
    systick_interrupt_enable();
    systick_counter_enable();
    
    
}

bool add_timed_callback(uint32_t offset, void (*_callback_function)(void *), void *_param, callback_id_t *pos){
	int i=0;
	while(i<NUM_CALLBACKS){
		struct timed_callback *cur_callback = &callbacks[i];
		if(cur_callback->is_stale){
			
			cur_callback->time = timer_ms + offset;
			bool wait_rollover = offset<=timer_ms;
			cur_callback->callback_function = _callback_function;
			cur_callback->param = _param;
			cur_callback->is_stale = false;
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
