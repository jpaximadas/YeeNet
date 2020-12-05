
#define _GNU_SOURCE
#include "uart.h"
#include <errno.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

FILE *fp_uart = NULL;  // run uart_setup to intialize

ssize_t _iord(void *_cookie, char *_buf, size_t _n) {
    /* dont support reading now */
    (void)_cookie;
    (void)_buf;
    (void)_n;
    return 0;
}

ssize_t _iowr(void *_cookie, const char *_buf, size_t _n) {
    uint32_t dev = (uint32_t)_cookie;

    int written = 0;
    while (_n-- > 0) {
        usart_send_blocking(dev, *_buf++);
        written++;
    };
    return written;
}

FILE *uart_setup(uint32_t usart) {
    cookie_io_functions_t stub = {_iord, _iowr, NULL, NULL};
    FILE *fp = fopencookie((void *)usart, "rw+", stub);
    /* Do not buffer the serial line */
    setvbuf(fp, NULL, _IONBF, 0);
    return fp;
}

uint8_t uart_read_until(uint32_t usart, uint8_t buf_out[256], int n, char last) {
    uint8_t pos = 0;
    bool reached_end = false;
    while (pos < n && !reached_end && pos != 255) {
        buf_out[pos] = usart_recv_blocking(usart);
        reached_end = (buf_out[pos] == last);
        pos++;
    }
    return pos;
}
