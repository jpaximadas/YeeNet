#include "platform/platform.h"
#include "address.h"

#include <libopencm3/stm32/gpio.h>
#include <sys/types.h>

static uint8_t my_addr;

uint8_t local_address_get(void){
	return my_addr;
}

void local_address_setup(void){
	bool bit0 = !!gpio_get(platform_pinout.address0.port, platform_pinout.address0.pin);
    bool bit1 = !!gpio_get(platform_pinout.address1.port, platform_pinout.address1.pin);
    my_addr = (bit1 << 1) | bit0;

    my_addr += 1; // Address should start at 1
}

