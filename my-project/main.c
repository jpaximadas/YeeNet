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

#include <string.h>
#include "uart.h"
#include "lora.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <libopencm3/stm32/spi.h>




static struct lora_modem lora0;

static struct pin_port rst = {.pin=GPIO9, .port=GPIOB};
static struct pin_port mosi = {.pin=GPIO5, .port=GPIOB};
static struct pin_port miso = {.pin=GPIO4, .port=GPIOB};
static struct pin_port sck = {.pin=GPIO3, .port=GPIOB};
static struct pin_port nss = {.pin=GPIO15, .port=GPIOA};
static struct pin_port irq = {.pin=GPIO8, .port=GPIOB};





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



FILE *fp_uart;

int main(void) {
	clock_setup();
    // Initalize USART0 at 115200 baud
    
    
    fp_uart = uart_setup();
    fprintf(fp_uart,"here I am\r\n");
    
    //lora_setup(&lora0,miso,mosi,sck,nss,rst,irq);
	for(;;); //halt
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
            fprintf(fp,"sent: %s\r\n",buf);
            lora_listen(&lora0);
        }

        enum lora_fifo_status msg_stat = lora_get_packet(&lora0,buf);
        //fprintf(&serial0->iostream,"packet stat: %x\r\n",msg_stat);

        if (msg_stat == FIFO_GOOD) {
            fprintf(fp,"got message: %s\r\n", buf);
        } else if(msg_stat == FIFO_BAD) {
            fprintf(fp, "bad packet\r\n");
        }
        _delay_ms(500);
    }*/
}
