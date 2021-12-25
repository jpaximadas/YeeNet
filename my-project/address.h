/**
 * @file address.h
 * @brief Provides a one byte address
 *
 * Currently, local_address_setup reads the state of two pins which can be controlled via external resistor.
 * This is usually useful for identifying modems when running tests.
 * Future additions will permit this value to be controlled through eight pins or through software.
 */
#pragma once
#include <sys/types.h>

/**
 * Returns the local address as initialized by local_address_setup
 *
 * @return an 8 bit address
 */
uint8_t local_address_get(void);

/**
 * Reads the address from the address pins and stores it in a variable.
 */
void local_address_setup(void);
