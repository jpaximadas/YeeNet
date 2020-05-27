
#include "address.h"

#include <libopencm3/stm32/gpio.h>
#include <sys/types.h>

uint8_t my_addr;

uint8_t local_address_get(void){
	return my_addr;
}

void local_address_setup(void){
	//assumes PORTB is already set up by modem_ll.c
	//TODO: reorganize how peripherals are managed
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);
	
	my_addr = 0;
	
	if(	gpio_get(GPIOB,GPIO10)==GPIO10 ){
		my_addr += 0x1;
	}
	if( gpio_get(GPIOB,GPIO11)==GPIO11){
		my_addr += 0x2;
	}
	
	my_addr++; //don't be 0
}

