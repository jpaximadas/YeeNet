/**
 * modem_hl.h
 * 
 * Define high-level functions for interacting with SX127x chip.
 * Alternate platforms must implement:
 * - modem_transmit()
 * - modem_listen()
 * - modem_load_payload()
 * - modem_get_payload()
 * - modem_setup()
 * - rand_32()
 */

#pragma once

#include "util.h"
#include "sx127x.h"

#include <stdbool.h>
#include <sys/types.h>
#include <stdlib.h>

struct modem_hw{
	
	uint32_t spi_interface;
	
	uint32_t rst_port;
	uint32_t rst_pin;
	
	uint32_t ss_port;
	uint32_t ss_pin;
	
	uint32_t mosi_port;
	uint32_t mosi_pin;
	
	uint32_t miso_port;
	uint32_t miso_pin;
	
	uint32_t sck_port;
	uint32_t sck_pin;
	
	uint32_t irq_port;
	uint32_t irq_pin;
    
};

enum irq_mode {
	RX_DONE,
	TX_DONE,
	INVALID
};

//defines all the data required to work a given modem/MCU platform
struct modem {

    volatile uint8_t irq_data;
    volatile bool irq_seen;
    enum irq_mode cur_irq_type;
    
	void * callback_arg;
    void (*rx_callback)(void *);
    void (*tx_callback)(void *);
    
    struct modem_hw *hw;
    struct modulation_config *modulation;

	uint32_t extra_time_ms; //expresses time taken by post/preprocessing steps on chip not accounted for by airtime calcs

        
};

//set up the modem
bool modem_setup(struct modem *this_modem, struct modem_hw *hw);

//atach callbacks
void modem_attach_callbacks(struct modem *this_modem, void (*_rx_callback)(void *), void (*_tx_callback)(void *), void *_callback_arg );

//set the payload for next TX
void modem_load_payload(struct modem *this_modem, uint8_t msg[MAX_PAYLOAD_LENGTH], uint8_t length);

//Transmit
void modem_transmit(struct modem *this_modem);

void modem_load_and_transmit(struct modem *this_modem, uint8_t msg[MAX_PAYLOAD_LENGTH], uint8_t length);

//Put the modem into RX mode
bool modem_listen(struct modem *this_modem);

//expresses that status of a data pulled from the fifo
enum payload_status {
    PAYLOAD_GOOD,
    PAYLOAD_BAD,
    PAYLOAD_EMPTY
};

//get the payload from last RX
enum payload_status modem_get_payload(struct modem *this_modem, uint8_t buf_out[MAX_PAYLOAD_LENGTH], uint8_t *length);

//pull a 32 bit integer from the pseudom-random number source
inline static uint32_t rand_32(void) {
    return rand();
}

//if the header is disabled, length is not accounted for
uint32_t modem_get_airtime_usec(struct modem *this_modem, uint8_t payload_length);

bool modem_is_clear(struct modem *this_modem);

int32_t get_last_payload_rssi(struct modem *this_modem);

double get_last_payload_snr(struct modem *this_modem);

bool payload_length_is_fixed(struct modem *this_modem);
