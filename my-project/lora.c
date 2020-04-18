#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

#include "lora.h"
#include "uart.h"
#include "util.h"


#define ARRAY_SIZE(x) (sizeof((x)) / sizeof(*(x)))

#define FXOSC 32000000
#define FREQ_TO_REG(in_freq) ((uint32_t)(( ((uint64_t)in_freq) << 19) / FXOSC))
#define REG_TO_FREQ(in_reg) ((uint32_t)((FXOSC*in_reg) >> 19))

// LoRA interrupt routine
static struct lora_modem *exti8_modem;
void exti8_isr(void) {
	exti_reset_request(EXTI8);
    // Read irq_data register
    exti8_modem->irq_data = lora_read_reg(exti8_modem, LORA_REG_IRQFLAGS);

    // Clear IRQ flag. Needs to be done twice for some reason (hw errata?)
    lora_write_reg(exti8_modem, LORA_REG_IRQFLAGS, 0xFF);
    lora_write_reg(exti8_modem, LORA_REG_IRQFLAGS, 0xFF);

    exti8_modem->irq_seen = false;
}

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

//
// High-level LoRA API
//

#define CUR_MODEM_CONFIG DEFAULT_MODEM_CONFIG


bool lora_setup(struct lora_modem *lora, 
	struct pin_port miso_pin_port, 
	struct pin_port mosi_pin_port, 
	struct pin_port sck_pin_port, 
	struct pin_port nss_pin_port, 
	struct pin_port rst_pin_port, 
	struct pin_port irq_pin_port) {
	fprintf(fp_uart,"entered lora setup");
    lora->irq_data = 0;
    lora->irq_seen = true;
    lora->miso_pin_port = miso_pin_port;
    lora->mosi_pin_port = mosi_pin_port;
    lora->sck_pin_port = sck_pin_port;
    lora->nss_pin_port = nss_pin_port;
    lora->rst_pin_port = rst_pin_port;
    lora->irq_pin_port = irq_pin_port;

    // Configure pin modes and spi interface
    gpio_setup(lora);
    spi_setup(lora);
    irq_setup(lora);
	
	fprintf(fp_uart,"completed mcu config steps");
    // TODO: Allow user-configurable interrupt pins
    // For now, assume exti8 is used
    exti8_modem = lora;

    // Change mode LORA + SLEEP
    if (!lora_write_reg_and_check(lora, LORA_REG_OP_MODE, MODE_LORA | MODE_SLEEP, true)) {
#ifdef DEBUG
        fputs("Failed to enter LORA+SLEEP mode!\r\n", fp_uart);
#endif
        return false;
    }

    // Set packet size
    if (!lora_write_reg_and_check(lora, LORA_REG_PAYLOAD_LENGTH, LORA_PACKET_SIZE, false) ||
        !lora_write_reg_and_check(lora, LORA_REG_MAX_PAYLOAD_LENGTH, LORA_PACKET_SIZE, false)) {
#ifdef DEBUG
        fprintf(fp_uart, "Failed to set payload size!\n");
#endif
        return false;
    }

    // Set registers
    for (size_t i=0; i<ARRAY_SIZE(CUR_MODEM_CONFIG); i++) {
        if (!lora_write_reg_and_check(lora, CUR_MODEM_CONFIG[i][0], CUR_MODEM_CONFIG[i][1], false)) {
#ifdef DEBUG
            fprintf(fp_uart, "Failed to write reg: 0x%x\r\n", CUR_MODEM_CONFIG[i][0]);
#endif
            return false;
        }
    }

#ifdef DEBUG
    fprintf(fp_uart,"Starting RNG generation\n\r");
#endif
    seed_random(lora);
    return true;
}

void seed_random(struct lora_modem *lora) {
    uint32_t new_seed = 0;
    lora_write_reg(lora,LORA_REG_OP_MODE,MODE_LORA | MODE_RXCON);
    for(uint8_t i = 0;i<32;i++){
        uint32_t val = lora_read_reg(lora, LORA_REG_RSSI_WIDEBAND);
        delay_nops(1000000);
        new_seed = new_seed | ( (val&0x00000001) << i);
    }
#ifdef DEBUG
    fprintf(fp_uart, "generated seed %lx\r\n",new_seed);
#endif
    srand((long) new_seed);
    lora_write_reg(lora,LORA_REG_OP_MODE,MODE_LORA | MODE_STDBY);
}

uint32_t rand_32(void) {
    return rand();
}

void lora_load_message(struct lora_modem *lora, uint8_t msg[LORA_PACKET_SIZE]) {
    lora_write_fifo(lora, msg, LORA_PACKET_SIZE, 0);
}

bool lora_transmit(struct lora_modem *lora) {
    // Configure DIO0 to interrupt on TXDONE, switch to TX mode
    lora_write_reg(lora, REG_DIO_MAPPING_1, 0x40 /* TXDONE */);
    lora_write_reg(lora, LORA_REG_OP_MODE, MODE_LORA | MODE_TX);
    uint8_t reg = lora_read_reg(lora,LORA_REG_OP_MODE);
    uint8_t reg2 = lora_read_reg(lora,0x12);

    // Wait for IRQ
    while (lora->irq_seen); 
    bool ret = lora->irq_data == LORA_MASK_IRQFLAGS_TXDONE;
    lora->irq_seen = true;

    return ret;
}

void lora_listen(struct lora_modem *lora) {
    lora_write_reg(lora, REG_DIO_MAPPING_1, 0x00 /* RXDONE */);
    lora_write_reg(lora,LORA_REG_OP_MODE,MODE_LORA | MODE_RXCON);
    lora->irq_seen = true; //reset irq flag
}

enum lora_fifo_status lora_get_packet(struct lora_modem *lora, uint8_t buf_out[LORA_PACKET_SIZE]) {
    uint8_t data = lora->irq_data;
    if (lora->irq_seen)
        return FIFO_EMPTY;
    lora->irq_seen = true;

    if (!(data & LORA_MASK_IRQFLAGS_RXDONE))
        return FIFO_BAD;

    if (data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR)
        return FIFO_BAD;

    // FIFO contains valid packet, return it
    uint8_t ptr = lora_read_reg(lora, LORA_REG_FIFO_RX_CUR_ADDR);
    lora_read_fifo(lora, buf_out, LORA_PACKET_SIZE, ptr);
    return FIFO_GOOD;
}

//
// Low-level LoRA API
//

void spi_setup(struct lora_modem *lora) {

  /* Configure GPIOs: SS=PA15, SCK=PB3, MISO=PB4 and MOSI=PB5 */
  gpio_set_mode(lora->nss_pin_port.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, lora->nss_pin_port.pin);
  gpio_set_mode(lora->sck_pin_port.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, lora->sck_pin_port.pin);
  gpio_set_mode(lora->mosi_pin_port.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, lora->mosi_pin_port.pin);
  gpio_set_mode(lora->miso_pin_port.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, lora->miso_pin_port.pin);

  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
  spi_reset(lora->spi_interface);

  /* Set up SPI as Master in mode 0
   */
  spi_set_master_mode(lora->spi_interface);
  spi_set_baudrate_prescaler(lora->spi_interface,SPI_CR1_BR_FPCLK_DIV_64);
  spi_set_standard_mode(lora->spi_interface,0);

  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
  spi_enable_software_slave_management(lora->spi_interface);
  spi_set_nss_high(lora->spi_interface);

  /* Enable SPI1 periph. */
  spi_enable(lora->spi_interface);
}

void gpio_setup(struct lora_modem *lora) {
	gpio_set_mode(lora->rst_pin_port.port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, lora->rst_pin_port.pin);
	
}

void irq_setup(struct lora_modem *lora){
	
	nvic_enable_irq(NVIC_EXTI8_IRQ); //interrupt on PB8
	gpio_set_mode(lora->irq_pin_port.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, lora->rst_pin_port.pin);
	exti_select_source(EXTI8,lora->irq_pin_port.port);
	exti_set_trigger(EXTI8,EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI8);
}

void lora_write_fifo(struct lora_modem *lora, uint8_t* buf, uint8_t len, uint8_t offset) {
    // Some assertions to check input data in DEBUG mode

    // Update modem's FIFO address pointer
    spi_set_nss_low(lora->spi_interface);
    spi_xfer(lora->spi_interface,LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(lora->spi_interface,offset);
    spi_set_nss_high(lora->spi_interface);
    //assume compiler is not good enough to emit an sbi instruction for the digital_writes
    // Write data to FIFO.
    spi_set_nss_low(lora->spi_interface);
    spi_xfer(lora->spi_interface,LORA_REG_FIFO | WRITE_MASK);
    for (uint8_t i=0; i<len; i++) {
        spi_xfer(lora->spi_interface,buf[i]);
    }
    spi_set_nss_high(lora->spi_interface);

}

void lora_read_fifo(struct lora_modem *lora, uint8_t *buf, uint8_t len, uint8_t offset) {

    // Update modem's FIFO address pointer
    spi_set_nss_low(lora->spi_interface);
    spi_xfer(lora->spi_interface,LORA_REG_FIFO_ADDR_PTR | WRITE_MASK);
    spi_xfer(lora->spi_interface,offset);
    spi_set_nss_high(lora->spi_interface);

    // Read data from FIFO
    spi_set_nss_low(lora->spi_interface);
    spi_xfer(lora->spi_interface,LORA_REG_FIFO);
    for (uint8_t i=0; i<len; i++) {
        buf[i] = spi_xfer(lora->spi_interface,0x00);
    }
    spi_set_nss_high(lora->spi_interface);
}

uint8_t lora_read_reg(struct lora_modem *lora, uint8_t reg) {
    spi_set_nss_low(lora->spi_interface);

    spi_xfer(lora->spi_interface,reg & 0x7F);
    uint8_t ret = spi_xfer(lora->spi_interface,0);

    spi_set_nss_high(lora->spi_interface);

    return ret;
}

void lora_write_reg(struct lora_modem *lora, uint8_t reg, uint8_t val) {
    spi_set_nss_low(lora->spi_interface);

    spi_xfer(lora->spi_interface, reg | WRITE_MASK);
    spi_xfer(lora->spi_interface, val);

    spi_set_nss_high(lora->spi_interface);
}

bool lora_write_reg_and_check(struct lora_modem *lora, uint8_t reg, uint8_t val, bool delay) {
    // Write register
    lora_write_reg(lora, reg, val);

    // Delay
    if (delay)
        delay_nops(10000000);

    //read reg
    uint8_t new_val = lora_read_reg(lora, reg);

    // Return whether write succeeded
    return val == new_val;
}

void lora_dbg_print_irq(uint8_t data) {
    if(data & LORA_MASK_IRQFLAGS_RXTIMEOUT)
        fputs("RX timed out\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_RXDONE)
        fputs("RX finished \r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_PAYLOADCRCERROR)
        fputs("CRC error\r\n", fp_uart);

    if(!(data & LORA_MASK_IRQFLAGS_VALIDHEADER))
        fputs("Last header invalid\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_TXDONE)
        fputs("TX complete\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_CADDONE)
        fputs("CAD finished\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_FHSSCHANGECHANNEL)
        fputs("FHSS change channel\r\n", fp_uart);

    if(data & LORA_MASK_IRQFLAGS_CADDETECTED)
        fputs("Channel activity detected\r\n", fp_uart);
}
