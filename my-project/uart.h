#pragma once
#include <sys/types.h>
#include <stdio.h>

FILE *fp_uart;

ssize_t _iord(void *_cookie, char *_buf, size_t _n);
ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);
FILE *uart_setup(void);
