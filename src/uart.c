#define _GNU_SOURCE
#include "otf_cobs.h"
#include "platform/platform.h"
#include "uart.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>

struct cobs_encode_buf usart1_cobs_tx_buffer = {.buftype = ENCODE_BUF};
struct cobs_decode_buf usart1_cobs_rx_buffer = {.buftype = DECODE_BUF};

uint16_t tx_buf_pos;
bool error_detected;
void usart1_isr(void) {
    // Process individual IRQs
    if (((USART_CR1(platform_pinout.p_usart) & USART_CR1_RXNEIE) != 0) &&
        ((USART_SR(platform_pinout.p_usart) & USART_SR_RXNE) != 0)) {
        enum cobs_decode_result ret =
            cobs_decode_buf_push_char(&usart1_cobs_rx_buffer, usart_recv(platform_pinout.p_usart));

        switch (ret) {
            case OK:
                break;
            case TERMINATED:
                usart_disable_rx_interrupt(platform_pinout.p_usart);
                // USART1_CR1 &= ~USART_CR1_RXNEIE;
                break;
            case OVERFLOW:
            case ENCODING_ERROR:
                cobs_buf_reset(&usart1_cobs_tx_buffer);  // may be unnecessary
                cobs_encode_buf_push_char(&usart1_cobs_tx_buffer, COBS_ERROR);
                cobs_encode_buf_terminate(&usart1_cobs_tx_buffer);
                usart1_release();
                break;
        }
    }
    if (((USART_CR1(platform_pinout.p_usart) & USART_CR1_TXEIE) != 0) &&
        ((USART_SR(platform_pinout.p_usart) & USART_SR_TXE) != 0)) {
        char c = usart1_cobs_tx_buffer.buf[tx_buf_pos];

        if (c == 0x00) {
            usart_disable_tx_interrupt(platform_pinout.p_usart);
            cobs_buf_reset(&usart1_cobs_tx_buffer);
        }
        tx_buf_pos++;
        usart_send(platform_pinout.p_usart, c);
    }
}

void usart1_host_link_init() {
    platform_usart_init();

    cobs_buf_reset(&usart1_cobs_rx_buffer);
    cobs_buf_reset(&usart1_cobs_tx_buffer);

    usart_enable_rx_interrupt(platform_pinout.p_usart);
}

ssize_t usart1_write_bytes(uint8_t *buf, size_t n) {
    return cobs_encode_buf_push(&usart1_cobs_tx_buffer, buf, n);
}

ssize_t usart1_write_byte(uint8_t c) {
    return cobs_encode_buf_push_char(&usart1_cobs_tx_buffer, c);
}

// let internal and external callers release the interface
// Note: be sure that tx buffer is empty by correctly terminating all packets before calling this function
void usart1_release() {
    cobs_encode_buf_terminate(&usart1_cobs_tx_buffer);  // cap off the cobs buffer
    cobs_buf_reset(&usart1_cobs_rx_buffer);             // reset the rx buffer
    error_detected = false;
    usart_enable_rx_interrupt(platform_pinout.p_usart);  // prepare for incoming bytes
    tx_buf_pos = 0;
    usart_enable_tx_interrupt(platform_pinout.p_usart);  // start the send process
}

// This will run even if the iface is unlocked
uint16_t usart1_get_command(uint8_t **buf_ptr) {
    uint16_t retval;
    if (usart1_cobs_rx_buffer.frame_is_terminated) {
        *buf_ptr = usart1_cobs_rx_buffer.buf;  // set the pointer to the cobs buffer
        return usart1_cobs_rx_buffer.pos;      // return the number of bytes available
    } else {
        return 0;
    }
}
