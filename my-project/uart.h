#pragma once
#include <libopencm3/stm32/usart.h>
#include <stdio.h>


FILE *usart1_setup(uint32_t baud);
//enabled using the file pointer
void usart1_terminate(void);

ssize_t usart1_write_bytes(uint8_t *buf, size_t n, bool terminate_when_done);

uint16_t usart1_get_command(uint8_t **buf_ptr);

void usart1_release(void);

inline static bool uart_available(uint32_t usart) {
    return USART_SR(usart) & USART_SR_RXNE;
}