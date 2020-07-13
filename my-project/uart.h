#pragma once
#include <sys/types.h>
#include <stdio.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>

#define COBS_TX_BUF_SIZE 300
#define COBS_RX_BUF_SIZE 300
#define ENCODING_OVH_START 1
#define ENCODING_OVH_END 1

enum cobs_buffer_type{
    RX_BUFFER,
    TX_BUFFER
};

struct cobs_tx_buffer{
	enum cobs_buffer_type buffer_type;
	uint8_t buf[COBS_TX_BUF_SIZE];
	uint16_t pos;
	uint16_t last_zero_pos;
};

struct cobs_rx_buffer{
	enum cobs_buffer_type buffer_type;
	uint8_t buf[COBS_RX_BUF_SIZE];
	uint16_t pos;
	uint16_t next_zero_pos;
	bool next_zero_is_overhead;
	bool frame_is_terminated;
};

FILE *usart1_setup(uint32_t baud);
//enabled using the file pointer
void usart1_terminate(void);

ssize_t usart1_write_bytes(uint8_t *buf, size_t n, bool terminate_when_done);

uint16_t usart1_get_command(uint8_t **buf_ptr);

void usart1_release(void);

inline static bool uart_available(void){
	return USART_SR(USART1) & USART_SR_RXNE;
}