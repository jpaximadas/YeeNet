#define DEBUG

#include "modem_hl.h"
#include "uart.h"
#include "util.h"
#include "callback_timer.h"
#include "address.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>

#include <stdbool.h>
#include <stdlib.h>

//debug
#include "modem_ll.h"
#include "sx127x.h"

uint8_t buf[255];

static struct modem lora0;

static struct modem_hw dev_breadboard = {
		
	.spi_interface = SPI1,
	
	.rst_port = GPIOB,
	.rst_pin = GPIO9,
	
	.mosi_port = GPIOA,
	.mosi_pin = GPIO7,
	
	.miso_port = GPIOA,
	.miso_pin = GPIO6,
	
	.sck_port = GPIOA,
	.sck_pin = GPIO5,
	
	.ss_port = GPIOA,
	.ss_pin = GPIO1,
	
	.irq_port = GPIOA,
	.irq_pin = GPIO0
	
};

void clock_setup(void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	
	//enable GPIOB clock
	rcc_periph_clock_enable(RCC_GPIOB);
	
	//enable GPIOC clock
	rcc_periph_clock_enable(RCC_GPIOC);

	rcc_periph_clock_enable(RCC_AFIO);

	/* Enable clocks for USART1. */
	rcc_periph_clock_enable(RCC_USART1);
	
	//enable clock for spi
	rcc_periph_clock_enable(RCC_SPI1);
	
	//enable clock for timer2
	rcc_periph_clock_enable(RCC_TIM2);
}


static void my_rx(struct modem *this_modem){
	//fprintf(fp_uart,"in rx isr\r\n");
	uint8_t recv_len;
	for(int i = 0; i<255;i++) buf[i] = 0;
	enum payload_status msg_stat = modem_get_payload(&lora0,buf,&recv_len);
	this_modem->irq_seen = true;
	if (msg_stat == PAYLOAD_GOOD) {
            fprintf(fp_uart,"got message: %s\r\n", buf);
            fprintf(fp_uart,"length: %u\r\n",recv_len);
            
    } else if(msg_stat == PAYLOAD_BAD) {
            fprintf(fp_uart, "bad packet\r\n");
    } else if(msg_stat == PAYLOAD_EMPTY){
			fprintf(fp_uart,"empty fifo\r\n");
	}
	
	if(msg_stat != PAYLOAD_EMPTY){
		fprintf(fp_uart,"RSSI: %ld dB\r\n",get_last_payload_rssi(&lora0));
        double snr = get_last_payload_snr(&lora0);
        int32_t snr_floored = (int32_t)snr;
        int32_t snr_decimals = abs((int32_t)((snr - ((double)snr_floored))*100.0));
        fprintf(fp_uart,"SNR: %ld.%ld dB\r\n",snr_floored,snr_decimals);
	}
    modem_listen(&lora0);
}

static void my_tx(struct modem *this_modem){
	this_modem->irq_seen = true;
	//fprintf(fp_uart,"mode at start of my_tx: %x\r\n",lora_read_reg(&lora0,LORA_REG_OP_MODE));
	if(this_modem->irq_data == LORA_MASK_IRQFLAGS_TXDONE){
		fprintf(fp_uart,"transmit was successful\r\n");
		//fprintf(fp_uart,"measured airtime: %lu us \r\n",get_last_airtime(this_modem));
	}else{
		fprintf(fp_uart,"transmit failed!\r\n");
	}
	modem_listen(&lora0);
	//fprintf(fp_uart,"mode at end of my_tx: %x\r\n",lora_read_reg(&lora0,LORA_REG_OP_MODE));
}

uint8_t send_len;

callback_id_t id;

char greeting[3] = "hi";


void hop(void *param){
	char *str = (char *) param;
	fprintf(fp_uart,"%s\r\n",str);
	add_timed_callback(1000, &hop, &greeting, &id);
}

int main(void) {
	
	clock_setup();
    fp_uart = uart_setup();
    fprintf(fp_uart,"start setup\r\n");
    callback_timer_setup();
    
    modem_setup(&lora0,&my_rx,&my_tx,&dev_breadboard);
    
	local_address_setup();
	fprintf(fp_uart,"Local address: %x\r\n",local_address_get());
    
    fprintf(fp_uart,"finished setup!\r\n");
    
    //hop(&greeting);
    
    modem_listen(&lora0);
    
    
    for(;;){
        //fprintf(fp_uart,"looping\r\n");
        
        if(uart_available()){
			
			for(int i = 0; i<255;i++) buf[i] = 0;
			send_len = uart_read_until(USART1, buf, sizeof(buf), '\r')-1;
			
			fprintf(fp_uart,"packet size: %u\r\n", send_len);
			fprintf(fp_uart,"est. airtime: %lu us\r\n", get_airtime(&lora0,send_len));
			if(modem_is_clear(&lora0)){
				modem_load_and_transmit(&lora0,buf,send_len);
			} else {
				fprintf(fp_uart,"modem is busy!\r\n");
			}
		}
		
		delay_nops(10000);
    }
}
