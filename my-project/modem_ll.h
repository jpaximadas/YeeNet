/**
 * modem_ll.h
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

enum lora_mode { SLEEP, STANDBY, RX, TX };

extern struct modem *volatile exti0_modem;  // must be volatile or will be optimized out

void irq_setup(struct modem *this_modem);

inline static void ss_set(struct modem *this_modem) {
    gpio_set(this_modem->ss.port, this_modem->ss.pin);
}

inline static void ss_clear(struct modem *this_modem) {
    gpio_clear(this_modem->ss.port, this_modem->ss.pin);
}

void lora_write_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset);
void lora_read_fifo(struct modem *this_modem, uint8_t *buf, uint8_t len, uint8_t offset);

void lora_config_modulation(struct modem *this_modem, struct modulation_config *modulation);
void lora_change_mode(struct modem *this_modem, enum lora_mode change_to);

uint8_t lora_read_reg(struct modem *this_modem, uint8_t reg);
void lora_write_reg(struct modem *this_modem, uint8_t reg, uint8_t val);
void lora_write_reg_and_check(struct modem *this_modem, uint8_t reg, uint8_t val, bool delay);

void lora_dbg_print_irq(uint8_t data);

// seed pseudo-random number source
void seed_random(struct modem *this_modem);
