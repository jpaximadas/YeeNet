#pragma once
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

extern FILE *fp_uart;  // run uart_setup to intialize

ssize_t _iord(void *_cookie, char *_buf, size_t _n);
ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);
FILE *uart_setup(uint32_t usart);

uint8_t uart_read_until(uint32_t usart, uint8_t *buf, int n, char last);

inline static bool uart_available(uint32_t usart) {
    return USART_SR(usart) & USART_SR_RXNE;
}
