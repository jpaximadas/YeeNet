#include "api.h"
#include "api-asm.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#define LITTLE_BIT 200000

static void clock_setup(void)
{
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Enable clocks for GPIO port A (for GPIO_USART2_TX) and USART2. */
}


static void gpio_setup(void)
{
	/* Set GPIO (in GPIO port A) to 'output push-pull'. */
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

int main(void) {
	/* add your own code */
    clock_setup();
    gpio_setup();
    gpio_set(GPIOC,GPIO13);
    for(;;){
        for (int i = 0; i < LITTLE_BIT; i++) {
		    	__asm__("nop");
	    }
        gpio_toggle(GPIOC,GPIO13);
    }
}
