#ifndef UART_H
#define UART_H
#include <sys/types.h>
#include <stdio.h>



ssize_t _iord(void *_cookie, char *_buf, size_t _n);
ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);
FILE *uart_setup(void);
#endif
