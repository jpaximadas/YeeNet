#pragma once
#include <libopencm3/stm32/usart.h>
#include <stdio.h>

void usart1_host_link_init();

ssize_t usart1_write_bytes(uint8_t *buf, size_t n);

ssize_t usart1_write_byte(uint8_t c);

uint16_t usart1_get_command(uint8_t **buf_ptr);

void usart1_release(void);

inline static bool uart_available(uint32_t usart) {
    return USART_SR(usart) & USART_SR_RXNE;
}