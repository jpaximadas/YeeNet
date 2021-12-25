/**
 * @file callback_timer.h
 * @brief Facilitates the delayed execution of a function and other timing functions
 *
 * The callback timer module is essentially a series of functions and data structures built around providing
 * functionality from ARM SysTick. SysTick is configured to call an ISR every millisecond. This interrupt
 * increments the timer_ms variable and checks if there are any functions that need to be run. It can also be
 * used to start and stop a timer to measure the quantity of time a procedure takes.
 */

#pragma once
#include <libopencm3/cm3/systick.h>
#include <sys/types.h>

/**
 * Handle provided upon creation of a timed callback. Assumed to be immutable.
 */
typedef uint8_t callback_id_t;

/**
 * Handle provided upon creation of a timed callback. Assumed to be immutable.
 */
typedef uint8_t timer_id_t;

/**
 * The number of milliseconds that have passed since the MCU started
 */
extern volatile uint32_t system_ms;

/**
 * Gets the value of the system time
 *
 * @return the system time
 */
static inline uint32_t get_system_ms_val(void) {
    return system_ms;
}

/**
 * @struct timed_callback
 *
 * Stores the information necessary to call a function after a perscribed number of milliseconds
 */
struct timed_callback {
    /**
     * Determines if an entry in the timed_callback array is in use
     */
    bool is_stale;
    /**
     * The time the callback function needs to execute
     */
    uint32_t time;
    /**
     * Whether the timed callback is waiting for system_ms to roll over
     */
    bool wait_rollover;
    /**
     * Function pointer to the callback function
     */
    void (*callback_function)(void *);
    /**
     * Argument to be passed to the callback function
     */
    void *param;
};

struct timer {
    /**
     * Whether an entry in the timer array is being used
     */
    bool is_stale;
    /**
     * When the timer was started
     */
    uint32_t start;
};

/**
 * Sets up the callback_timer. Should be run during init.
 */
void callback_timer_setup(void);

/**
 * Given a timed_callback handle, will disable it.
 * Useful for disabling a timed_callback before it gets a chance to run.
 *
 * @param pos the handle for the timed callback
 */
void remove_timed_callback(callback_id_t pos);

/**
 * Will run a function in a given number of milliseconds in the future.
 *
 * @param offset number of milliseconds to wait
 * @param _callback_function pointer to the callback function
 * @param _param the parameter to be passed to the callback function
 * @param pos pointer to callback_id_t variable that will be populated with a valid handle
 * @return whether or not there was space in the array for adding a callback
 */
bool add_timed_callback(uint32_t offset,
                        void (*_callback_function)(void *),
                        void *_param,
                        callback_id_t *pos);

/**
 * Starts a timer
 *
 * @param pos the timer position in the array
 * @return true if the position is available and false it it wasn't
 */
bool start_timer(uint8_t pos);

/**
 * Stops a timer
 * @param pos the timer position in the array
 * @return the number of milliseconds that have passed since the start of the timer
 */
uint32_t stop_timer(uint8_t pos);