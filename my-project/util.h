#pragma once
#include <sys/types.h>

inline static void delay_nops(uint32_t nops) {
    for (uint32_t i = 0; i < nops; i++) {
        __asm__("nop");
    }
}

/**
 * Aggregate containing information required to address a GPIO pin.
 * Since the total size of the struct is only 64 bits, it's fine to pass
 * this around by value.
 */
typedef struct pin_descriptor {
    uint32_t port;
    uint32_t pin;
} pin_descriptor_t;
