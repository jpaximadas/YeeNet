
#include <sys/types.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "util.h"

volatile uint32_t ms;
void sys_tick_handler(void)
{
	ms++;

}

void timer_setup(void){
	ms = 0;
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(71999);
    systick_interrupt_enable();
    systick_counter_enable();
}

