/**
 * @file modem_hl.h
 *
 * Define high-level functions for interacting with SX127x chip.
 * Alternate platforms must implement:
 * - modem_transmit()
 * - modem_listen()
 * - modem_load_payload()
 * - modem_get_payload()
 * - modem_setup()
 * - rand_32()
 */

#pragma once

#include "sx127x.h"
#include "util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * Represents the irq_mode
 */
enum irq_mode { RX_DONE, TX_DONE, INVALID };

/**
 * @struct modem
 *
 * Represents a modem. Currently only supports SX1276
 */
struct modem {
    /**
     * Stores IRQ flags when interrupt is executed
     */
    volatile uint8_t irq_data;
    /**
     * Indicates if the IRQ had been responded to
     */
    volatile bool irq_seen;
    /**
     * Expresses which mode the IRQ is in
     */
    enum irq_mode cur_irq_type;
    /**
     * Argument to be passed to callback functions
     */
    void *callback_arg;
    /**
     * Function pointer to RX callback function
     */
    void (*rx_callback)(void *);
    /**
     * Function pointer to TX callback function.
     */
    void (*tx_callback)(void *);
    /**
     * libopencm3 spi interface number
     */
    uint32_t spi_interface;
    /**
     * Reset pin from platform.h
     */
    pin_descriptor_t rst;
    /**
     * Slave select pin from platform.h
     */
    pin_descriptor_t ss;
    /**
     * Records the current modulation settings
     */
    struct modulation_config *modulation;
    /**
     * Fudge factor to compensate for Semtech provided airtime calculation underestimating real airtime
     * Necessary for packet_handler operation
     */
    uint32_t extra_time_ms;
};

/**
 * @brief set up the modem and associated MCU peripherals
 *
 * @param this_modem pointer to struct representing the modem
 * @param spi_interface libopencm3 spi interface
 * @param ss slave select for the modem
 * @param rst reset pin for the modem
 * @return true if setup was successful, false if not
 */
bool modem_setup(struct modem *this_modem, uint32_t spi_interface, pin_descriptor_t ss, pin_descriptor_t rst);

/**
 * @brief attach callbacks to a modem
 *
 * @param this_modem pointer to struct representing the modem
 * @param _rx_callback function to be run after RX is done
 * @param _tx_callback function to be run after TX is done
 * @param _callback_arg void pointer to be passed to both callback functions
 */
void modem_attach_callbacks(struct modem *this_modem,
                            void (*_rx_callback)(void *),
                            void (*_tx_callback)(void *),
                            void *_callback_arg);

/**
 * @brief load a payload into the modem FIFO
 *
 * @param this_modem pointer to struct representing the modem
 * @param msg pointer to the payload
 * @param length length of the payload
 * @return false if the packet is greater than the fixed length in implicit header mode, true otherwise
 */
bool modem_load_payload(struct modem *this_modem, uint8_t *msg, uint8_t length);

/**
 * @brief put the modem into TX mode
 * This function will not block. The DIO0 interrupt will be configured to TX Done.
 *
 * @param this_modem pointer to struct representing the modem
 */
void modem_transmit(struct modem *this_modem);

/**
 * @brief load a payload and put the modem into TX mode
 * This function will not block. The DIO0 interrupt will be configured to TX Done.
 *
 * @param this_modem pointer to struct representing the modem
 * @param msg pointer to the payload
 * @param length the length of the payload
 * @return false if the packet is greater than the fixed length in implicit header mode, true otherwise
 */
bool modem_load_and_transmit(struct modem *this_modem, uint8_t *msg, uint8_t length);

/**
 * @brief put the modem into RX mode
 * This function will not block. The DIO0 interrupt will be configured to RX done.
 *
 * @param this_modem pointer to struct representing the modem
 */
void modem_listen(struct modem *this_modem);

/**
 * @brief put the modem into standby mode
 * The modem can neither receive nor transmit in this mode.
 *
 * @param this_modem pointer to struct representing the modem
 */
void modem_standby(struct modem *this_modem);

/**
 * Expresses status of a payload pulled out of the modem's FIFO
 */
enum payload_status {
    /**
     * Payload is OK. If CRC is enabled it passed.
     */
    PAYLOAD_GOOD,
    /**
     * Payload is bad. CRC is enabled and it failed.
     */
    PAYLOAD_BAD,
    /**
     * A packet is not present.
     */
    PAYLOAD_EMPTY
};

/**
 * @brief get a payload out of the modem
 *
 * @param this_modem pointer to struct representing the modem
 * @param buf_out location to write the fifo contents
 * @param length pointer to location to write the length of the packet in the buffer
 * @return information about the packet that is in the buffer (see enum documentation)
 */
enum payload_status modem_get_payload(struct modem *this_modem,
                                      uint8_t buf_out[MAX_PAYLOAD_LENGTH],
                                      uint8_t *length);

/**
 * @brief generate 32 random bits
 *
 * @return an unsigned 32 bit integer composed of random bits
 */
inline static uint32_t rand_32(void) {
    return rand();
}

/**
 * @brief compute the airtime of a packet under the current modulation configuration
 * This function is intended to mirror Semtech's.
 * These results have been shown to underestimate the true airtime.
 *
 * @param this_modem pointer to struct representing the modem
 * @param payload_length length of a packet. This only matters in explicit header mode where the payload
 * length can vary.
 * @return airtime in microseconds.
 */
uint32_t modem_get_airtime_usec(struct modem *this_modem, uint8_t payload_length);

/**
 * @brief checks if the modem is clear
 * This function checks the modem status register for the "signal detected" and "signal synced" bits.
 * The modem must be in RX mode for this to be meaningful as a sense-before-transmit utility.
 * Using channel activity detection (CAD) mode may be an option in the future although changing modes will
 * disrupt packet reception.
 *
 * @param this_modem pointer to struct representing the modem
 * @return true if modem is not actively transmiting or receiving, false if it is
 */
bool modem_is_clear(struct modem *this_modem);

/**
 * @brief reads the RSSI of the last packet
 * This function will get the RSSI of the last packet.
 * Note that this function assumes use of the HF port of the SX1276 chip.
 * This function uses a method that is not accurate for RSSI above -100dBm.
 * Check SX1276 datasheet section 5.5.5 for more info.
 *
 * @param this_modem pointer to struct representing the modem
 * @return the RSSI of the last packet in dBm
 */
int32_t modem_get_last_payload_rssi(struct modem *this_modem);

/**
 * @brief reads the SNR of the last packet
 * This function will obtain the SNR of the last packet.
 *
 * @param this_modem pointer to modem struct
 * @return the SNR as a floating point value
 */
float modem_get_last_payload_snr(struct modem *this_modem);

/**
 * @return indicates if the payload length is fixed
 */
bool payload_length_is_fixed(struct modem *this_modem);
