
#include "callback_timer.h"
#include "uart.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <sys/types.h>

/**
 * Sets the number of callbacks that the callback_timer will manage.
 * Can be increased or decreased as needed.
 */
#define NUM_CALLBACKS 6
/**
 * Declares the array where timed_callbacks will be stored.
 */
struct timed_callback callbacks[NUM_CALLBACKS];

/**
 * Sets the number of timers the callback_timer will manage.
 * Can be increased or decreased as needed.
 */
#define NUM_TIMERS 6
/**
 * Declares the array where timers will be stored.
 */
struct timer timers[NUM_TIMERS];

/**
 * Keeps track of the number of milliseconds that have passed since startup.
 */
volatile uint32_t system_ms;

/**
 * Interrupt routine for SysTick.
 * callback_timer_setup configures the MCU to run this every millisecond
 * Each time it runs:
 * -system_ms is incremented
 * -each element in the callback array is checked to see if it needs to be run
 */
void sys_tick_handler(void) {
    system_ms++;

    for (int i = 0; i < NUM_CALLBACKS; i++) {
        if (system_ms == 0) {
            callbacks[i].wait_rollover = false;
        }  // if a rollover occurs, remove rollover waiting

        if (!(callbacks[i].is_stale) && callbacks[i].time <= system_ms && !(callbacks[i].wait_rollover)) {
            callbacks[i].is_stale = true;  // mark this struct as stale
            (*(callbacks[i].callback_function))(
                callbacks[i].param);  // execute the function while passing param
        }
    }
}

void callback_timer_setup(void) {
    system_ms = 0;
    for (int i = 0; i < NUM_CALLBACKS; i++) {
        callbacks[i].is_stale = true;
    }
    for (int i = 0; i < NUM_TIMERS; i++) {
        timers[i].is_stale = true;
    }
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(71999);
    systick_interrupt_enable();
    systick_counter_enable();
}

bool add_timed_callback(uint32_t offset,
                        void (*_callback_function)(void *),
                        void *_param,
                        callback_id_t *pos) {
    int i = 0;
    while (i < NUM_CALLBACKS) {
        if (callbacks[i].is_stale) {
            callbacks[i].time = system_ms + offset;
            bool wait_rollover = offset <= system_ms;  // I think offset might need to be callbacks[i].time
            callbacks[i].callback_function = _callback_function;
            callbacks[i].param = _param;
            callbacks[i].is_stale = false;
            *pos = (callback_id_t)i;  // report array position

            return true;
        }
        i++;
    }
    return false;
}

void remove_timed_callback(callback_id_t pos) {
    callbacks[(uint8_t)pos].is_stale = true;
}

bool start_timer(uint8_t pos) {
    if (timers[pos].is_stale) {
        timers[pos].start = system_ms;
        timers[pos].is_stale = false;
        return true;
    }
    return false;
}

uint32_t stop_timer(uint8_t pos) {
    timers[pos].is_stale = true;
    timers[pos].start = system_ms - timers[pos].start;
    return timers[pos].start;
}