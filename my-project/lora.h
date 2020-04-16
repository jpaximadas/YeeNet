#pragma once
#include <stdbool.h>

#define LORA_PACKET_SIZE 255

#define WRITE_MASK 0x80

#define LORA_REG_FIFO 0x00
#define LORA_REG_OP_MODE 0x01
// OP_MODE flags
#define MODE_LORA 0x80
#define MODE_SLEEP 0x0
#define MODE_STDBY 0x1
#define MODE_FSTX  0x2
#define MODE_TX    0x3
#define MODE_FSRX  0x4
#define MODE_RXCON 0x5
#define MODE_CAD   0x7

#define LORA_REG_FR_MSB 0x6
#define LORA_REG_FR_MID 0x7
#define LORA_REG_FR_LSB 0x8
#define LORA_REG_PA_CONFIG 0x9
#define LORA_REG_PA_RAMP 0xA
#define LORA_REG_OCP 0xB
#define LORA_REG_LNA 0xC
#define LORA_REG_FIFO_ADDR_PTR 0xD
#define LORA_REG_FIFO_TX_BASE_ADDR 0xE
#define LORA_REG_FIFO_RX_BASE_ADDR 0xF
#define LORA_REG_FIFO_RX_CUR_ADDR 0x10

#define LORA_REG_IRQFLAGS 0x12
#define LORA_MASK_IRQFLAGS_RXTIMEOUT 0b10000000
#define LORA_MASK_IRQFLAGS_RXDONE 0b01000000
#define LORA_MASK_IRQFLAGS_PAYLOADCRCERROR 0b00100000
#define LORA_MASK_IRQFLAGS_VALIDHEADER 0b00010000
#define LORA_MASK_IRQFLAGS_TXDONE 0b00001000
#define LORA_MASK_IRQFLAGS_CADDONE 0b00000100
#define LORA_MASK_IRQFLAGS_FHSSCHANGECHANNEL 0b00000010
#define LORA_MASK_IRQFLAGS_CADDETECTED 0b00000001

#define LORA_REG_MODEMSTAT 0x18
#define LORA_MASK_MODEMSTAT_CLEAR 0b00010000
#define LORA_MASK_MODEMSTAT_VALIDHEADER 0b00001000
#define LORA_MASK_MODEMSTAT_RXONGOING 0b00000100
#define LORA_MASK_MODEMSTAT_SYNCHRYONIZED 0b00000010
#define LORA_MASK_MODEMSTAT_DETECTED 0b00000001

#define LORA_REG_MODEM_CONFIG_1 0x1D
#define LORA_REG_MODEM_CONFIG_2 0x1E
#define LORA_REG_SYMB_TIMEOUT   0x1F
#define LORA_REG_PREAMBLE_MSB   0x20
#define LORA_REG_PREAMBLE_LSB   0x21
#define LORA_REG_PAYLOAD_LENGTH 0x22
#define LORA_REG_MAX_PAYLOAD_LENGTH 0x23
#define LORA_REG_HOP_PERIOD 0x24

#define LORA_REG_RSSI_WIDEBAND 0x2C

#define LORA_REG_DETECT_OPTIMIZE 0x31

#define LORA_REG_DETECTION_THRESHOLD 0x37


#define REG_DIO_MAPPING_1 0x40
#define REG_DIO_MAPPING_2 0x41
#define REG_PA_DAC 0x4D


struct pin_port {
	uint32_t port;
	uint32_t pin;
};

struct lora_modem {

    volatile uint8_t irq_data;
    volatile bool irq_seen;
    uint32_t spi_interface;
    struct pin_port rst_pin_port;
    struct pin_port nss_pin_port;
    struct pin_port mosi_pin_port;
    struct pin_port miso_pin_port;
    struct pin_port sck_pin_port;
    
};

// High level API
enum lora_fifo_status {
    FIFO_GOOD,
    FIFO_BAD,
    FIFO_EMPTY
};

/*
bool lora_setup(struct lora_modem *lora, 
	struct pin_port miso_pin_port, 
	struct pin_port mosi_pin_port, 
	struct pin_port sck_pin_port, 
	struct pin_port nss_pin_port, 
	struct pin_port rst_pin_port, 
	struct pin_port irq_pin_port);*/
void lora_load_message(struct lora_modem *lora, uint8_t msg[LORA_PACKET_SIZE]);
bool lora_transmit(struct lora_modem *lora);
void lora_listen(struct lora_modem *lora);
enum lora_fifo_status lora_get_packet(struct lora_modem *lora, uint8_t buf_out[LORA_PACKET_SIZE]);

// Low level API
void lora_write_fifo(struct lora_modem *lora, uint8_t *buf, uint8_t len, uint8_t offset);
void lora_read_fifo(struct lora_modem *lora, uint8_t *buf, uint8_t len, uint8_t offset);


uint8_t lora_read_reg(struct lora_modem *lora, uint8_t reg);
void lora_write_reg(struct lora_modem *lora, uint8_t reg, uint8_t val);
bool lora_write_reg_and_check(struct lora_modem *lora, uint8_t reg, uint8_t val, bool delay);

void seed_random(struct lora_modem *lora);
uint32_t rand_32(void);

void lora_dbg_print_irq(uint8_t data);
