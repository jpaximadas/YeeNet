#define DEBUG

#include "modem_hl.h"

#include "modem_ll.h"
#include "uart.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <libopencm3/stm32/gpio.h>

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof(*(x)))

#define FXOSC 32000000
#define FREQ_TO_REG(in_freq) ((uint32_t)(( ((uint64_t)in_freq) << 19) / FXOSC))
#define REG_TO_FREQ(in_reg) ((uint32_t)((FXOSC*in_reg) >> 19))

#define CUR_MODEM_CONFIG DEFAULT_MODEM_CONFIG

static const uint8_t LONGR_MODEM_CONFIG[][2] = {
    // Configure PA_BOOST with max power
    {LORA_REG_PA_CONFIG, 0x8f},

    // Enable overload current protection with max trim
    {LORA_REG_OCP, 0x3f},

    // Set RegPaDac to 0x87
    {REG_PA_DAC, 0x87},

    // Set the FIFO RX/TX pointers to 0
    {LORA_REG_FIFO_TX_BASE_ADDR, 0x00},
    {LORA_REG_FIFO_RX_BASE_ADDR, 0x00},

    // Set RegModemConfig1 as follows:
    // Bw = 0b1001 (500kHz)
    // CodingRate = 0b100 (4/8)
    // ImplicitHeader = 1
    {LORA_REG_MODEM_CONFIG_1, 0x99},

    // Set RegModemConfig2 as follows:
    // SpreadingFactor = 12
    // TxContinuousMode = 0
    // RxPayloadCrcOn = 1
    {LORA_REG_MODEM_CONFIG_2, 0xc4},

    // Set preamble length to 16
    {LORA_REG_PREAMBLE_MSB, 0x00},
    {LORA_REG_PREAMBLE_LSB, 0x08},

    // Set payload length and max length to 15
    //{LORA_REG_PAYLOAD_LENGTH, 0x0f},
    //{LORA_REG_MAX_PAYLOAD_LENGTH, 0x0f},

    // Disable frequency hopping
    {LORA_REG_HOP_PERIOD, 0x00},

    // Set DetectionThreshold to 0xA (for SF > 6)
    {LORA_REG_DETECTION_THRESHOLD, 0x0A},

    // Set DetectionOptimize to 0x3 (for SF > 6)
    {LORA_REG_DETECT_OPTIMIZE, 0x03},

    // Set the frequency to 915MHz
    // We can use FREQ_TO_REG() here because it is declared as `constexpr`
    // and can therefore be evaluated at compile-time
    {LORA_REG_FR_MSB, (FREQ_TO_REG(915000000) >> 16) & 0b11111111},
    {LORA_REG_FR_MID, (FREQ_TO_REG(915000000) >> 8) & 0b11111111},
    {LORA_REG_FR_LSB, FREQ_TO_REG(915000000) & 0b11111111},
};

static const uint8_t DEFAULT_MODEM_CONFIG[][2] = {
    // Configure PA_BOOST with max power
    {LORA_REG_PA_CONFIG, 0x8f},

    // Enable overload current protection with max trim
    {LORA_REG_OCP, 0x3f},

    // Set RegPaDac to 0x87 (requried for SF=6)
    {REG_PA_DAC, 0x87},

    // Set the FIFO RX/TX pointers to 0
    {LORA_REG_FIFO_TX_BASE_ADDR, 0x00},
    {LORA_REG_FIFO_RX_BASE_ADDR, 0x00},

    // Set RegModemConfig1 as follows:
    // Bw = 0b1001 (500kHz)
    // CodingRate = 0b001 (4/5)
    // ImplicitHeader = 1
    {LORA_REG_MODEM_CONFIG_1, 0x93},

    // Set RegModemConfig2 as follows:
    // SpreadingFactor = 6
    // TxContinuousMode = 0
    // RxPayloadCrcOn = 1
    {LORA_REG_MODEM_CONFIG_2, 0x64},

    // Set preamble length to 8
    {LORA_REG_PREAMBLE_MSB, 0x00},
    {LORA_REG_PREAMBLE_LSB, 0x08},

    // Set payload length and max length to 255
    //{LORA_REG_PAYLOAD_LENGTH, 0x0f},
    //{LORA_REG_MAX_PAYLOAD_LENGTH, 0x0f},

    // Disable frequency hopping
    {LORA_REG_HOP_PERIOD, 0x00},

    // Set DetectionThreshold to 0xC (for SF = 6)
    {LORA_REG_DETECTION_THRESHOLD, 0x0c},

    // Set DetectionOptimize to 0x5 (for SF = 6)
    {LORA_REG_DETECT_OPTIMIZE, 0x05},

    // Set the frequency to 915MHz
    // We can use FREQ_TO_REG() here because it is declared as `constexpr`
    // and can therefore be evaluated at compile-time
    {LORA_REG_FR_MSB, (FREQ_TO_REG(915000000) >> 16) & 0b11111111},
    {LORA_REG_FR_MID, (FREQ_TO_REG(915000000) >> 8) & 0b11111111},
    {LORA_REG_FR_LSB, FREQ_TO_REG(915000000) & 0b11111111},
};


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
	
	lora_write_reg(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_SLEEP);
	
	
	//fprintf(fp_uart,"attempting to put lora into sleep mode\r\n");
    // Change mode LORA + SLEEP
    if (!lora_write_reg_and_check(this_modem, LORA_REG_OP_MODE, MODE_LORA | MODE_SLEEP, true)) {
#ifdef DEBUG
        fputs("Failed to enter LORA+SLEEP mode!\r\n", fp_uart);
#endif
        return false;
    }
	
    // Set packet size
    if (!lora_write_reg_and_check(this_modem, LORA_REG_PAYLOAD_LENGTH, LORA_PACKET_SIZE, false) ||
        !lora_write_reg_and_check(this_modem, LORA_REG_MAX_PAYLOAD_LENGTH, LORA_PACKET_SIZE, false)) {
#ifdef DEBUG
        fprintf(fp_uart, "Failed to set payload size!\n");
#endif
        return false;
    }

    // Set registers
    for (size_t i=0; i<ARRAY_SIZE(CUR_MODEM_CONFIG); i++) {
        if (!lora_write_reg_and_check(this_modem, CUR_MODEM_CONFIG[i][0], CUR_MODEM_CONFIG[i][1], false)) {
#ifdef DEBUG
            fprintf(fp_uart, "Failed to write reg: 0x%x\r\n", CUR_MODEM_CONFIG[i][0]);
#endif
            return false;
        }
    }

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
void modem_load_payload(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE]) {
	lora_change_mode(this_modem,STANDBY);
    lora_write_fifo(this_modem, msg, LORA_PACKET_SIZE, 0);
}

bool modem_transmit(struct modem *this_modem) {
    this_modem->irq_seen = true;
    return lora_change_mode(this_modem,TX);
}

bool modem_load_and_transmit(struct modem *this_modem, uint8_t msg[LORA_PACKET_SIZE]) {
    modem_load_payload(this_modem,msg);
    return modem_transmit(this_modem);
}

bool modem_listen(struct modem *this_modem) {
	this_modem->irq_seen = true; //reset irq flag
	return lora_change_mode(this_modem,RX);
}

//this function will put the lora into a standby mode
enum payload_status modem_get_payload(struct modem *this_modem, uint8_t buf_out[LORA_PACKET_SIZE]) {
	
    uint8_t data = this_modem->irq_data;
    if (this_modem->irq_seen || (this_modem->cur_irq_type != RX_DONE)){
        return PAYLOAD_EMPTY;
	}
	lora_change_mode(this_modem,STANDBY); //put the lora into standby mode for reg read
	enum payload_status retval = PAYLOAD_GOOD;
	
    if (!(data & LORA_MASK_IRQFLAGS_RXDONE))
        retval = PAYLOAD_EMPTY;

    if (data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR)
        retval = PAYLOAD_BAD;

    // FIFO contains valid packet, return it
    uint8_t ptr = lora_read_reg(this_modem, LORA_REG_FIFO_RX_CUR_ADDR);
    //lora_write_reg(this_modem, LORA_REG_OP_MODE,MODE_LORA | MODE_STDBY);
    lora_read_fifo(this_modem, buf_out, LORA_PACKET_SIZE, ptr);
    return retval;
}
