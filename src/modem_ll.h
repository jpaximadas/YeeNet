/**
 * @file modem_ll.h
 *
 * Defines low level functions for operating the SX126x chip.
 * Alternate platforms may name functions and create data structures at their own discretion.
 */

#pragma once

#include "modem_hl.h"
#include "sx127x.h"
#include "util.h"
#include <libopencm3/stm32/gpio.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * Enum to represent the LoRa modem's mode
 */
enum lora_mode { SLEEP, STANDBY, RX, TX };

/**
 * Modem struct for the DIO0 ISR to use
 */
extern struct modem *volatile exti0_modem;  // must be volatile or will be optimized out

/**
 * @brief set the slave select pin for a modem
 *
 * @param this_modem pointer to modem struct
 */
inline static void ss_set(struct modem *this_modem) {
    gpio_set(this_modem->ss.port, this_modem->ss.pin);
}

/**
 * @brief clear the slave select pin for a mode
 *
 * @param this_modem pointer to modem struct
 */
inline static void ss_clear(struct modem *this_modem) {
    gpio_clear(this_modem->ss.port, this_modem->ss.pin);
}

/**
 * @brief write to the LoRa FIFO
 *
 * @param this_modem pointer to modem struct
 * @param buf pointer to bytes to be written to the FIFO
 * @param len number of bytes to be written to the FIFO
 * @param trailing_zeros extra zeros to tack on after the bytes being written
 * @param offset ?
 */
void lora_write_fifo(struct modem *this_modem,
                     uint8_t *buf,
                     uint8_t len,
                     uint8_t trailing_zeros,
                     uint8_t offset);
void lora_read_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset);

void lora_config_modulation(struct modem *this_modem);
void lora_change_mode(struct modem *this_modem, enum lora_mode change_to);

uint8_t lora_read_reg(struct modem *this_modem, uint8_t reg);
void lora_write_reg(struct modem *this_modem, uint8_t reg, uint8_t val);
bool lora_write_reg_and_check(struct modem *this_modem, uint8_t reg, uint8_t val);

void lora_dbg_print_irq(uint8_t data);

// seed pseudo-random number source
void seed_random(struct modem *this_modem);
