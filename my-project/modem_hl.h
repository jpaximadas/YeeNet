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
    
    void (*rx_done_isr)(struct modem *);
    void (*tx_done_isr)(struct modem *);
    
    struct modem_hw *hw;
    struct modulation_config *modulation;
    
    uint32_t last_airtime;
        
};

//set up the modem
bool modem_setup(
	struct modem *this_modem, 
	void (*this_rx_isr)(struct modem *),
	void (*this_tx_isr)(struct modem *),
	struct modem_hw *hw
	);

//set the payload for next TX
void modem_load_payload(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE], uint8_t length);

//Transmit
bool modem_transmit(struct modem *this_modem);

bool modem_load_and_transmit(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE], uint8_t length);

//Put the modem into RX mode
bool modem_listen(struct modem *this_modem);

//expresses that status of a data pulled from the fifo
enum payload_status {
    PAYLOAD_GOOD,
    PAYLOAD_BAD,
    PAYLOAD_EMPTY
};

//get the payload from last RX
enum payload_status modem_get_payload(struct modem *this_modem, uint8_t buf_out[LORA_PACKET_SIZE], uint8_t *length);

//pull a 32 bit integer from the pseudom-random number source
uint32_t rand_32(void);

//if the header is disabled, length is not accounted for
uint32_t get_airtime(struct modem *this_modem, uint8_t length);

uint32_t get_last_airtime(struct modem *this_modem);
