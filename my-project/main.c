#include "host_link.h"
#include "platform/platform.h"
#include <stdbool.h>

int main(void) {
	
	platform_clocks_init();
    platform_gpio_init();

    host_link_init(USART);
	for(;;){
		host_link_parse();
	}
}
