#pragma once
#include <sys/types.h>
#include <stdio.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>

extern FILE *fp_uart; //run uart_setup to intialize

ssize_t _iord(void *_cookie, char *_buf, size_t _n);
ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);
FILE *uart_setup(void);

void uart_read_until(uint32_t usart, uint8_t *buf, int n, char last);

inline static bool uart_available(){
	return USART_SR(USART1) & USART_SR_RXNE;
}
