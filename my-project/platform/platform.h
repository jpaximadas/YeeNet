#pragma once

#include "util.h"

#include <stdint.h>

/**
 * Definitions of platform-specific low-level functions (GPIO setup, clock setup, peripherals, etc.)
 */

//
// Pinouts/Board definitions
//

/**
 * Platform-specific pinout table
 */
struct platform_pinout_table {
    // Peripherals
    uint32_t p_spi;
    uint32_t p_usart;

    // Modem pins
    pin_descriptor_t modem_rst;
    pin_descriptor_t modem_ss;
    pin_descriptor_t modem_mosi;
    pin_descriptor_t modem_miso;
    pin_descriptor_t modem_sck;
    pin_descriptor_t modem_irq;

    // Address pins
    pin_descriptor_t address0;
    pin_descriptor_t address1;
};

extern const struct platform_pinout_table platform_pinout;

//
// Setup functions
//

void platform_clocks_init(void);
void platform_usart_init(void);
void platform_gpio_init(void);
void platform_spi_init(void);
void platform_irq_init(void);
