#include "address.h"
#include "platform/platform.h"
#include <libopencm3/stm32/gpio.h>
#include <sys/types.h>

/**
 * Stores the 8 bit address
 */
static uint8_t my_addr;

uint8_t local_address_get(void) {
    return my_addr;
}

void local_address_setup(void) {
    bool bit0 = !!gpio_get(platform_pinout.address0.port, platform_pinout.address0.pin);  // get the first bit
    bool bit1 = !!gpio_get(platform_pinout.address1.port, platform_pinout.address1.pin);  // get the second
                                                                                          // bit
    my_addr = (bit1 << 1) | bit0;                                                         // combine them
    my_addr += 1;  // Address should start at 1
}
