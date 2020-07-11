#pragma once
#include <sys/types.h>
#include <stdio.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>

extern FILE *fp_uart; //run uart_setup to initialize

#define TX_BUFF_SIZE 300
#define ENCODING_OVH_START 1
#define ENCODING_OVH_END 1

struct cobs_tx_buffer{
	uint8_t buf[TX_BUFF_SIZE];
	uint16_t pos;
	uint16_t last_zero_pos;
};

struct cobs_rx_buffer{
	uint8_t buf[RX_BUFF_SIZE];
	uint16_t pos;
	uint16_t last_zero_pos;
};

struct host_interface{
	FILE *tx;
	void (*get_command)(void);
}

FILE *usart1_setup(uint32_t baud);

//enabled using the file pointer
void usart1_terminate(void);

bool command_available()

uint8_t uart_read_until(uint32_t usart, uint8_t *buf, int n, char last);

inline static bool uart_available(void){
	return USART_SR(USART1) & USART_SR_RXNE;
}
