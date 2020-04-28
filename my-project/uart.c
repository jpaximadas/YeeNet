
#define _GNU_SOURCE
#include "uart.h"


#include <stdbool.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include <sys/types.h>
#include <libopencm3/stm32/gpio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

//static ssize_t _iord(void *_cookie, char *_buf, size_t _n);
//static ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);

FILE *fp_uart = NULL; //run uart_setup to intialize


ssize_t _iord(void *_cookie, char *_buf, size_t _n)
{
	/* dont support reading now */
	(void)_cookie;
	(void)_buf;
	(void)_n;
	return 0;
}

ssize_t _iowr(void *_cookie, const char *_buf, size_t _n)
{
	uint32_t dev = (uint32_t)_cookie;

	int written = 0;
	while (_n-- > 0) {
		usart_send_blocking(dev, *_buf++);
		written++;
	};
	return written;
}

FILE *uart_setup(void) {
	/* Enable the USART1 interrupt. */
	//nvic_enable_irq(NVIC_USART1_IRQ);

	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);

	/* Enable USART1 Receive interrupt. */
	//USART_CR1(USART1) |= USART_CR1_RXNEIE;

	/* Finally enable the USART. */
	usart_enable(USART1);
	
	cookie_io_functions_t stub = { _iord, _iowr, NULL, NULL };
	FILE *fp = fopencookie((void *)USART1, "rw+", stub);
	/* Do not buffer the serial line */
	setvbuf(fp, NULL, _IONBF, 0);
	return fp;
}

uint8_t uart_read_until(uint32_t usart, uint8_t buf_out[256], int n, char last){
	uint8_t pos = 0;
	bool reached_end = false;
	while(pos < n && !reached_end && pos!=255){
		buf_out[pos] = usart_recv_blocking(usart);
		reached_end = (buf_out[pos] == last);
		pos++;
		
	}
	return pos;
	
}
