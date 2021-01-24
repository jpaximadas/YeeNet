#define _GNU_SOURCE
#include "uart.h"
#include "otf_cobs.h"
#include "platform.h"
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>
#include <libopencm3/cm3/nvic.h>

#define GOOD_RDY_BYTE 0x00 //last cobs message was good, ready for next
#define BAD_RDY_BYTE 0x01 //last cobs message was bad, ready for next

struct cobs_encode_buffer usart1_cobs_tx_buffer = {
	.buffer_type = ENCODE_BUF
};
struct cobs_decode_buffer usart1_cobs_rx_buffer = {
	.buffer_type = DECODE_BUF
};

uint16_t tx_buf_pos;
void usart1_isr(void){
	uint32_t serviced_irqs = 0;
 
     // Process individual IRQs
     if (uart_is_interrupt_source(platform_pinout.p_usart, UART_INT_RX)) {
        enum cobs_decode_result ret = cobs_decode_buf_push_char(&usart1_cobs_tx_buffer,usart_recv(platform_pinout.p_usart));
		switch(ret){
			case OK:
				break;
			case TERMINATED:
				usart_disable_rx_interrupt(platform_pinout.p_usart);
				break;
			case OVERFLOW:
				//send an error
				break;
			case ENCODING_ERROR:
				//send an error
				break;
		}
        serviced_irqs |= UART_INT_RX;
     }
     if (uart_is_interrupt_source(platform_pinout.p_usart, UART_INT_TX)) {
		char c = usart1_cobs_tx_buffer.buf[tx_buf_pos];
        
		if(c==0x00){
			usart_disable_tx_interrupt(platform_pinout.p_usart);
			cobs_buf_reset(&usart1_cobs_tx_buffer);
		}
		tx_buf_pos++;
		usart_send(platform_pinout.p_usart,c);
        serviced_irqs |= UART_INT_TX;
     }
 
     // Clear the interrupt flag for the processed IRQs
     uart_clear_interrupt_flag(platform_pinout.p_usart, serviced_irqs);
	
}

void usart1_host_link_init() {

	platform_usart_init();

	cobs_buffer_reset(&usart1_cobs_rx_buffer);
	cobs_buffer_reset(&usart1_cobs_tx_buffer);

	usart_enable_rx_interrupt(platform_pinout.p_usart);

}

static ssize_t usart1_write_bytes(uint8_t *_buf, size_t _n) {
	return cobs_encode_buf_push(&usart1_cobs_tx_buffer, _buf, _n);
}

//let internal and external callers release the interface
//Note: be sure that tx buffer is empty by correctly terminating all packets before calling this function
void usart1_release(){
	cobs_encode_buf_terminate(usart1_cobs_tx_buffer); //cap off the cobs buffer
	cobs_buffer_reset(&usart1_cobs_rx_buffer); //reset the rx buffer
	usart_enable_rx_interrupt(platform_pinout.p_usart); //prepare for incoming bytes
	tx_buf_pos = 0;
	usart_enable_tx_interrupt(platform_pinout.p_usart);//start the send process
	//TODO: enable TX interrupt, TX interrupt will self disable and reset the TX buffer
}

//This will run even if the iface is unlocked
uint16_t usart1_get_command(uint8_t **buf_ptr){
	uint16_t retval;
	if(usart1_cobs_rx_buffer.frame_is_terminated){
		*buf_ptr = usart1_cobs_rx_buffer.buf; //set the pointer to the cobs buffer
		return usart1_cobs_rx_buffer.pos; //return the number of bytes available
	} else {
		return 0;
	}
}

