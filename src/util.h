#pragma once
#include "host_link.h"
#include <sys/types.h>

__attribute__((noreturn)) void assert_fail(void);

#define ASSERT(x)                                                                   \
    do {                                                                            \
        if (!(x)) {                                                                 \
            fprintf(link.fp, "%s:%d: Assertion failed %s", __FILE__, __LINE__, #x); \
            assert_fail();                                                          \
        }                                                                           \
    } while (0)

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
