#define DEBUG

#include "modem_hl.h"

#include "modem_ll.h"
#include "modem_ll_config.h"
#include "uart.h"
#include "util.h"

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <libopencm3/stm32/gpio.h>

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof(*(x)))

#define FXOSC 32000000
#define FREQ_TO_REG(in_freq) ((uint32_t)(( ((uint64_t)in_freq) << 19) / FXOSC))
#define REG_TO_FREQ(in_reg) ((uint32_t)((FXOSC*in_reg) >> 19))

bool modem_setup(
	struct modem *this_modem, 
	void (*this_rx_isr)(struct modem *),
	void (*this_tx_isr)(struct modem *),
	struct modem_hw *hw
	) {
	
	//fprintf(fp_uart,"in modem_setup\r\n");
	
	//set up irq info in struct
    this_modem->irq_data = 0;
    this_modem->irq_seen = true;
    
	//fprintf(fp_uart,"configured struct irq data\r\n");
    
    //store config info in struct
    this_modem->hw = hw;
	
	//fprintf(fp_uart,"set hw pointer\r\n");
	
	//set rx/tx done isr function pointers
	this_modem->rx_done_isr = this_rx_isr;
	this_modem->tx_done_isr	= this_tx_isr;
	
	//fprintf(fp_uart,"set function pointers\r\n");
	
    // Configure pin modes and spi interface
    gpio_setup(this_modem);
    //fprintf(fp_uart,"gpio setup complete\r\n");
    spi_setup(this_modem);
    //fprintf(fp_uart,"spi setup complete\r\n");
    irq_setup(this_modem);
    //fprintf(fp_uart,"irq_setup complete\r\n");
    
    //fprintf(fp_uart,"hw setup complete\r\n");
    
    // TODO: Allow user-configurable interrupt pins
    // For now, assume exti0 is used on A0
    exti0_modem = this_modem;
	
	//fprintf(fp_uart,"exti0 pointer stored\r\n");
	
	//Reset the board
	gpio_clear(this_modem->hw->rst_port,this_modem->hw->rst_pin);
	delay_nops(1000000);
	gpio_set(this_modem->hw->rst_port,this_modem->hw->rst_pin);
	
	//fprintf(fp_uart,"lora board reset");
	//fprintf(fp_uart,"attempting to put lora into sleep mode\r\n");
	if(!lora_change_mode(this_modem,SLEEP)){
		#ifdef DEBUG
        fputs("Failed to enter LORA+SLEEP mode!\r\n", fp_uart);
		#endif
	}
	
    
	
	//assume max power settings
    //Configure PA_BOOST with max power
    lora_write_reg_and_check(this_modem,LORA_REG_PA_CONFIG, 0x8f,true);
    // Enable overload current protection with max trim
    lora_write_reg_and_check(this_modem,LORA_REG_OCP, 0x3f,true);
    // Set RegPaDac to 0x87, increases power?
    lora_write_reg_and_check(this_modem,REG_PA_DAC, 0x87,true);

    // Set the FIFO RX/TX pointers to 0
    lora_write_reg(this_modem,LORA_REG_FIFO_TX_BASE_ADDR, 0x00);
    lora_write_reg(this_modem,LORA_REG_FIFO_RX_BASE_ADDR, 0x00);

    // Disable frequency hopping
    lora_write_reg(this_modem,LORA_REG_HOP_PERIOD, 0x00);

    //Set frequency
    /*
    lora_write_reg(this_modem,LORA_REG_FR_MSB, (FREQ_TO_REG(915000000) >> 16) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_MID, (FREQ_TO_REG(915000000) >> 8) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_LSB, FREQ_TO_REG(915000000) & 0b11111111);
    */
    lora_write_reg(this_modem,LORA_REG_FR_MSB, (FREQ_TO_REG(915000000) >> 16) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_MID, (FREQ_TO_REG(915000000) >> 8) & 0b11111111);
    lora_write_reg(this_modem,LORA_REG_FR_LSB, FREQ_TO_REG(915000000) & 0b11111111);
    fprintf(fp_uart,"starting modulation config\r\n");
    lora_config_modulation(this_modem,&default_modulation);
    

#ifdef DEBUG
    fprintf(fp_uart,"Starting RNG generation\n\r");
#endif
    seed_random(this_modem);
    return true;
}

uint32_t rand_32(void) {
    return rand();
}

//this function will put the lora into a standby mode
void modem_load_payload(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE], uint8_t length) {
	lora_change_mode(this_modem,STANDBY);
	
	 if(this_modem->modulation->header_enabled){ //explicit header mode
        lora_write_reg(this_modem,LORA_REG_PAYLOAD_LENGTH, length); //inform lora of payload length
        lora_write_fifo(this_modem, msg, LORA_PACKET_SIZE, 0);
    } else { //fixed length, implicit mode
#ifdef DEBUG
        if(length>this_modem->modulation->payload_length){
            fprintf(fp_uart,"input to load_payload too long--excess will not be transmitted");
        }
#endif
		lora_write_fifo(this_modem, msg, this_modem->modulation->payload_length, 0);
    }

    
}

bool modem_transmit(struct modem *this_modem) {
    this_modem->irq_seen = true;
    return lora_change_mode(this_modem,TX);
}

bool modem_load_and_transmit(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE], uint8_t length) {
    modem_load_payload(this_modem,msg,length);
    return modem_transmit(this_modem);
}

bool modem_listen(struct modem *this_modem) {
	this_modem->irq_seen = true; //reset irq flag
	return lora_change_mode(this_modem,RX);
}

//this function will put the lora into a standby mode
enum payload_status modem_get_payload(struct modem *this_modem, uint8_t buf_out[LORA_PACKET_SIZE], uint8_t *length) {
	
    uint8_t data = this_modem->irq_data;
    if (this_modem->irq_seen || (this_modem->cur_irq_type != RX_DONE)){
        return PAYLOAD_EMPTY;
	}
	lora_change_mode(this_modem,STANDBY); //put the lora into standby mode for reg read
	enum payload_status retval = PAYLOAD_GOOD;
	
    if (!(data & LORA_MASK_IRQFLAGS_RXDONE)){
        retval = PAYLOAD_EMPTY;
	}
	
    if (data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR){
        retval = PAYLOAD_BAD;
	}
	
	if (this_modem->modulation->header_enabled) { //explicit header mode, variables length
		*length = lora_read_reg(this_modem,LORA_REG_RX_NB_BYTES);
	} else { //implicit header mode, fixed length
		*length = this_modem->modulation->payload_length;
	}
	
    // FIFO contains valid packet, return it
    uint8_t ptr = lora_read_reg(this_modem, LORA_REG_FIFO_RX_CUR_ADDR);
    //lora_write_reg(this_modem, LORA_REG_OP_MODE,MODE_LORA | MODE_STDBY);
    lora_read_fifo(this_modem, buf_out, *length, ptr);
    return retval;
}

uint32_t get_airtime(struct modem *this_modem, uint8_t length){
	
}
