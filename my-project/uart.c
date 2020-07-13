
#define _GNU_SOURCE

#include "uart.h"
#include <stdbool.h>
#include <stdio.h>
#include <libopencm3/stm32/usart.h>
#include <sys/types.h>
#include <libopencm3/stm32/gpio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

struct cobs_tx_buffer usart1_cobs_tx_buffer = {
	.buffer_type = TX_BUFFER
};
struct cobs_rx_buffer usart1_cobs_rx_buffer = {
	.buffer_type = RX_BUFFER
};

volatile bool dma1_in_use = false;

//general purpose usart write function
static ssize_t usart_write(struct cobs_tx_buffer *tx_buf, uint8_t *_buf, size_t _n) {
	uint16_t i = 0;

	while( dma1_in_use || !usart_get_flag(USART1,USART_SR_TXE) ){} //block while usart/DMA is busy writing the buffer

	while(tx_buf->pos < COBS_TX_BUF_SIZE-ENCODING_OVH_END && i<_n){

		if( (tx_buf->pos - tx_buf->last_zero_pos) != 0xFF ){
			//overhead byte not needed
			if(_buf[i]){
				//next byte in input buffer is not zero
				tx_buf->buf[tx_buf->pos] = _buf[i]; //write it
				//move to next character in the input buffer
			}else{
				//next byte in intput buffer is zero
				tx_buf->buf[tx_buf->last_zero_pos] = tx_buf->pos - tx_buf->last_zero_pos; //fill in the last zero position with the distance to this zero
				tx_buf->last_zero_pos = tx_buf->pos; //record the position of this next zero

			}
			i++; //move ahead to the next bute in the input buffer
			tx_buf->pos++; //move ahead to the next byte in the tx buffer
		} else {
			//254 bytes of nonzero characters precede this point in the buffer; a non-data overhead byte is needed
			tx_buf->buf[tx_buf->last_zero_pos] = 0xFF; //fill in the last zero position with the distance to this overhead byte
			tx_buf->last_zero_pos = tx_buf->pos; //record the position of this overhead byte
			tx_buf->pos++; //move ahead in the tx buffer but not the input buffer
		}

	}
	
	return i;
}

//wrapper for libc iostream
static ssize_t iowr(void *_cookie, const char *_buf, size_t _n){
	return usart_write((struct cobs_tx_buffer *) _cookie,(uint8_t *) _buf, _n);
}

void usart1_terminate(void){
	//cap off the tx buffer
	usart1_cobs_tx_buffer.buf[usart1_cobs_tx_buffer.pos] = 0x00;
	usart1_cobs_tx_buffer.buf[usart1_cobs_tx_buffer.last_zero_pos]  = usart1_cobs_tx_buffer.pos - usart1_cobs_tx_buffer.last_zero_pos;


	while( dma1_in_use || !usart_get_flag(USART1,USART_SR_TXE) ){}
	dma1_in_use = true;

	dma_channel_reset(DMA1, DMA_CHANNEL4);

	dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&USART1_DR);
	dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)usart1_cobs_tx_buffer.buf);
	dma_set_number_of_data(DMA1, DMA_CHANNEL4, usart1_cobs_tx_buffer.pos+1);
	dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
	dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_VERY_HIGH);

	dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

	dma_enable_channel(DMA1, DMA_CHANNEL4);

    usart_enable_tx_dma(USART1);
}

 //wrapper function for host_link
ssize_t usart1_write_bytes(uint8_t *buf, size_t n, bool terminate_when_done){
	ssize_t retval = usart_write(&usart1_cobs_tx_buffer, buf, n);
	if(terminate_when_done){
		usart1_terminate();
	}
	return retval;
}

static void cobs_buffer_reset(void *cobs_buffer){
	enum cobs_buffer_type buf_type = *((uint8_t *)cobs_buffer);

	switch(buf_type){
		case TX_BUFFER:
			{
				struct cobs_tx_buffer *buf = (struct cobs_tx_buffer *) cobs_buffer;
				buf->last_zero_pos = 0; //let writer write over the first byte to indicate position of next zero
				buf->pos = ENCODING_OVH_START; //start writing at beginning of data section
				break;
			}
		case RX_BUFFER:
			{
				struct cobs_rx_buffer *buf = (struct cobs_rx_buffer *) cobs_buffer;
				buf->next_zero_pos = 0;
				buf->pos = 0;
				buf->next_zero_is_overhead = true;
				buf->frame_is_terminated = false;
				break;
			}
	}
}

void dma1_channel4_isr(void)
{
	if ((DMA1_ISR &DMA_ISR_TCIF4) != 0) {
		DMA1_IFCR |= DMA_IFCR_CTCIF4;

	}

	dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

	usart_disable_tx_dma(USART1);

	dma_disable_channel(DMA1, DMA_CHANNEL4);

	dma1_in_use = false;

	cobs_buffer_reset(&usart1_cobs_tx_buffer);
}



FILE *usart1_setup(uint32_t baud) {
	//init cobs tx buffer
	rcc_periph_clock_enable(RCC_USART1);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, baud);
	usart_set_databits(USART1,8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);

	//enable DMA channel 4 for USART1 TX
	nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 0);
	nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);

	cobs_buffer_reset(&usart1_cobs_tx_buffer);

	//enable usart receive interrupt
	nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 0);
	nvic_enable_irq(NVIC_USART1_IRQ);
	usart_enable_rx_interrupt(USART1);
	cobs_buffer_reset(&usart1_cobs_rx_buffer);

	/* Finally enable the USART. */
	usart_enable(USART1);
	
	
	

	cookie_io_functions_t stub = { NULL, iowr, NULL, NULL };
	FILE *fp = fopencookie((void *) &usart1_cobs_tx_buffer, "w+", stub);

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

uint8_t err_chars[5] = {'E','R','R','\r','\n'};
void usart1_isr(void){

	uint8_t c = usart_recv(USART1);

	if(c == 0x00){
		if( usart1_cobs_rx_buffer.next_zero_pos == usart1_cobs_rx_buffer.pos){
			usart_disable_rx_interrupt(USART1); //disable usart interrupt after frame termination
			usart1_cobs_rx_buffer.frame_is_terminated = true;
			return;
		} else {
			usart1_write_bytes(err_chars, 5, true);
			cobs_buffer_reset(&usart1_cobs_rx_buffer);
			return;
		}
		
	}

	if(!(usart1_cobs_rx_buffer.pos<COBS_RX_BUF_SIZE)){
		usart1_write_bytes(err_chars, 5, true);
		cobs_buffer_reset(&usart1_cobs_rx_buffer);
		return;
	}

	if(usart1_cobs_rx_buffer.next_zero_pos == usart1_cobs_rx_buffer.pos){ //at a zero position
		usart1_cobs_rx_buffer.next_zero_pos += c; //update where the next zero position is

		if(!usart1_cobs_rx_buffer.next_zero_is_overhead){ //this zero position is not an overhead byte
			usart1_cobs_rx_buffer.buf[usart1_cobs_rx_buffer.pos] = 0x00; //fill in the zero
			usart1_cobs_rx_buffer.pos++; //move to the next character in the buffer
		}
		//if this zero position was overhead, the buffer does not move forward and nothing is written

		//determine if the following zero position is overhead
		if(c == 0xFF){
			usart1_cobs_rx_buffer.next_zero_is_overhead = true;
		} else {
			usart1_cobs_rx_buffer.next_zero_is_overhead = false;
		}
		
	} else {	//not at a zero position
		usart1_cobs_rx_buffer.buf[usart1_cobs_rx_buffer.pos] = c;
		usart1_cobs_rx_buffer.pos++;
	}
	
}

uint16_t usart1_get_command(uint8_t **buf_ptr){
	if(usart1_cobs_rx_buffer.frame_is_terminated){
		*buf_ptr = usart1_cobs_rx_buffer.buf; //set the pointer to the cobs buffer
		uint16_t retval = usart1_cobs_rx_buffer.pos+1; //return the number of bytes available
	} else {
		return 0;
	}
	
}

uint8_t ready_chars[5] = {'R','D','Y','\r','\n'};
void usart1_release(void){
	cobs_buffer_reset(&usart1_cobs_rx_buffer);
	usart_enable_rx_interrupt(USART1);
	usart1_write_bytes(ready_chars, 5, true);
}