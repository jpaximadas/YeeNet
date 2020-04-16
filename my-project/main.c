/**
 * Copyright 2019 Shawn Anastasio
 *
 * This file is part of libminiavr.
 *
 * libminiavr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libminiavr is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libminiavr.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * This file contains a short example of using libminiavr's UART
 * facilities with both the low-level I/O API and the C stdio API.
 *
 * After uploading the program, interact with it using GNU screen:
 * $ screen /dev/ttyX 115200 8n1
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "lora.h"

/*
#define RST_PORT GPIOB
#define RST_PIN GPIO9

#define SS_PORT GPIOA
#define SS_PIN GPIO15

#define SCK_PORT GPIOB
#define SCK_PIN GPIO3

#define MISO_PORT GPIOB
#define MISO_PIN GPIO4

#define MOSI_PORT GPIOB
#define MOSI_PIN GPIO5

#define IRQ_PORT GPIOB
#define IRQ_PIN GPIO8
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>


#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

static struct lora_modem lora0;

static struct pin_port rst = {.pin=GPIO9, .port=GPIOB};
static struct pin_port mosi = {.pin=GPIO5, .port=GPIOB};
static struct pin_port miso = {.pin=GPIO4, .port=GPIOB};
static struct pin_port sck = {.pin=GPIO3, .port=GPIOB};
static struct pin_port nss = {.pin=GPIO15, .port=GPIOA};
static struct pin_port irq = {.pin=GPIO8, .port=GPIOB};

static ssize_t _iord(void *_cookie, char *_buf, size_t _n);
static ssize_t _iowr(void *_cookie, const char *_buf, size_t _n);

static ssize_t _iord(void *_cookie, char *_buf, size_t _n)
{
	/* dont support reading now */
	(void)_cookie;
	(void)_buf;
	(void)_n;
	return 0;
}

static ssize_t _iowr(void *_cookie, const char *_buf, size_t _n)
{
	uint32_t dev = (uint32_t)_cookie;

	int written = 0;
	while (_n-- > 0) {
		usart_send_blocking(dev, *_buf++);
		written++;
	};
	return written;
}



static void clock_setup(void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	
	//enable GPIOB clock
	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_clock_enable(RCC_AFIO);

	/* Enable clocks for USART1. */
	rcc_periph_clock_enable(RCC_USART1);
	
	//enable clock for spi
	rcc_periph_clock_enable(RCC_SPI1);
}

static FILE *usart_setup(void) {
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



int main(void) {
	clock_setup();
    // Initalize USART0 at 115200 baud
    FILE *fp;
    
    fp = usart_setup();
    fprintf(fp,"here I am\r\n");
    
    
    for(;;); //halt
    
    //lora_setup(&lora0,miso,mosi,sck,nss,rst,irq);
	/*
    uint8_t buf[255];
    lora_listen(&lora0);
    for(;;){
        for(int i = 0; i<255;i++) buf[i] = 0;
        if(serial_available(serial0)!=0){
            uint8_t n = serial_read_until(serial0, buf, sizeof(buf), '\r');
            lora_load_message(&lora0,buf);


            // Wait for IRQ


            //fprintf(&serial0->iostream,"transmiting\r\n");
            lora_transmit(&lora0);
            fprintf(&serial0->iostream,"sent: %s\r\n",buf);
            lora_listen(&lora0);
        }

        enum lora_fifo_status msg_stat = lora_get_packet(&lora0,buf);
        //fprintf(&serial0->iostream,"packet stat: %x\r\n",msg_stat);

        if (msg_stat == FIFO_GOOD) {
            fprintf(&serial0->iostream,"got message: %s\r\n", buf);
        } else if(msg_stat == FIFO_BAD) {
            fprintf(&serial0->iostream, "bad packet\r\n");
        }
        _delay_ms(500);
    }*/
}
