/**
 * @file uart.h
 *
 * This file interacts with the COBS buffers to operate the UART interface.
 */
#pragma once
#include <libopencm3/stm32/usart.h>
#include <stdio.h>

/**
 * @brief initializes the uart interface so the host link can use it
 * The UART ISR is enabled here and the cobs buffers are reset
 */
void usart1_host_link_init();

/**
 * @brief write bytes to the UART interface
 *
 * This function is to be used by the host link and makes use of the COBS library for framing.
 * Since there isn't a way to tell if the frame is terminated from the input bytes, input bytes are buffered
 * until the interface is realeased.
 *
 * @param buf pointer to bytes to be written
 * @param n number of bytes to be written
 * @return number of bytes that were written
 *
 */
ssize_t usart1_write_bytes(uint8_t *buf, size_t n);

/**
 * @brief write a byte to the UART interface
 *
 * @param c byte to be written
 * @return number of bytes that were written
 */
ssize_t usart1_write_byte(uint8_t c);

/**
 * @brief check if a command has come in from the UART
 *
 * This function is meant to be used by the host link to check if a command has come in from the interface.
 *
 * @param buf_ptr pointer to where the caller want the pointer to the decoded frame written
 * @return number of bytes in the frame
 */
uint16_t usart1_get_command(uint8_t **buf_ptr);

/**
 * @brief indicate that the host link is done writing to the interface
 *
 * This function will terminate the outgoing COBS frame and enable the UART ISR in transmit mode
 */
void usart1_release(void);

inline static bool uart_available(uint32_t usart) {
    return USART_SR(usart) & USART_SR_RXNE;
}