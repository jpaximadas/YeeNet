/**
 * @file packet_handler.h
 * @brief Provides a reliable interface for transmitting and receiving packets.
 *
 * The packet handler layer manages a modem on a half-duplex connection.
 * It performs a few important tasks that makes using a modem more reliable and eliminates timing
 * considerations from the design of higher layers:
 *  - Prevents transmission during reception of a packet
 *  - Detects whether a packet was successfully (method for this depends on whether the packet was
 * ACKED/UNACKED)
 *  - Responds to detected failures of packet transmission in PERSISTENT modem in a way that resolves
 * collisions (exponential backoff)
 *  - Prevents further tranmissions if a packet is in the process of being sent
 *  - Automatically sends ACKS for packets that mandate one and NACKS when reception errors nack_occur
 *  - Completes tranmissions of a packet in a finite amount of time while providing indication whether packet
 * tranmission was successful
 *
 *
 */

#include "address.h"
#include "callback_timer.h"
#include "modem_hl.h"
#include "packet.h"
#include "uart.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <uart.h>
#include <util.h>

/**
 * Enum for indicating CSMA mode
 */
enum send_mode {

    CSMA_CD_1_PERSISTENT = 0,

    CSMA_CD_NON_PERSISTENT
};

/**
 * Enum for expressing the status of the last transmitted packet.
 */
enum send_outcome {
    /**
     * The handler concludes that the receiver got the last packet.
     */
    SUCCESS = 0,
    /**
     * The handler concludes that the receiver did not get the last packet.
     */
    STALE,
    FAILED_MAX_BACKOFFS_REACHED,
    FAILED_BACK,
    FAILED_TIMEOUT
};

/**
 * @struct packet_handler
 *
 * Defines a packet_handler
 */
struct packet_handler {
    /** Pointer to modem used by a packet_handler */
    struct modem *my_modem;

    /** Address of the node the packet_handler is operating on */
    uint8_t my_addr;

    /** indicates whether the handler is ready for a new packet */
    bool is_busy;

    /** indicates status of last packet that was sent */
    enum send_outcome last_send_outcome;

    enum send_mode my_send_mode;

    /** <b>internal variable</b> indicates if NACK was received when attempting the tranmission of a packet */
    bool nak_occurred;

    uint8_t log2CWcur;

    uint8_t log2CWmax;

    uint8_t log2CWmin;

    /** <b>internal variable</b> ID for adding and removing callback_timer for timeouts and retransmissions */
    callback_id_t my_timed_callback;

    /** <b>internal variable</b> removes need to recompute packet airtime after first transmit */
    uint32_t pkt_airtime_ms;

    /** <b>internal variable</b> indicates packets length of current outgoing packet */
    uint8_t pkt_length;

    /** pointer that is moved to location of current outgoing packet on call to handler_request_transmit */
    struct payload_record *tx_pkt;

    void (*send_function)(struct packet_handler *);

    /** pointer to location of where incoming packets should be written to */
    struct payload_record *rx_pkt;

    /** function pointer for callback executed after a data packet is received and processed */
    struct payload_record *(*pkt_rdy_callback)(void *);

    /** void pointer to data meant to be the argument to the callback function */
    void *callback_arg;

    /** <b>internal variable</b> ACK packet with blank destination meant to be populated upon use */
    struct packet_ack my_ack;

    /** <b>internal variable</b> NACK packet for use when the modem reports an error */
    struct packet_nak my_nak;

    /** A sort of NACK packet that indicates buffer full*/
    struct packet_bak my_bak;

    /** <b>internal variable</b>  precomputed ack airtime */
    uint32_t ack_airtime_ms;

    /** <b>internal variable</b> precomputed nack airtime */
    uint32_t nak_airtime_ms;

    uint32_t bak_airtime_ms;

    uint32_t timeout_ms;
};

extern struct packet_handler handler0;

/**

 */
void handler_setup(struct packet_handler *this_handler,
                   struct modem *_my_modem,
                   struct payload_record *_rx_pkt,
                   bool (*_pkt_rdy_callback)(void *),
                   void (*_handler_finished_callback)(void *),
                   void *_callback_arg,
                   enum send_mode _init_send_mode,
                   uint8_t _log2CWmin,
                   uint8_t _log2CWmax);

/**
 * Attempt to transmit a packet.
 *
 * Packet handler will check two things before transmitting:
 *  - Whether or not the handler is LOCKED
 *  - Whether or not the modem is busy
 *
 * If both checks pass, the packet handler is reset for a new packet, the packet is transmitted, and true is
 * returned. If either check fails, false is returned. packet_handler->my_state can be polled to avoid
 * performing a function call however even if the handler state is UNLOCKED this does not preclude the
 * possibility that the modem will be busy.
 *
 * @param this_handler pointer to handler struct
 * @param pkt pointer to packet ot be transmitted
 * @return whether or not the packet handler is starting the transmission
 */
bool handler_request_transmit(struct packet_handler *this_handler, struct payload_record *pkt);

static void handler_backoff_retransmit(void *param);

static inline void handler_rx_cleanup(struct packet_handler *this_handler, bool tx_on_cleanup) {
    if (tx_on_cleanup) {
        modem_transmit(this_handler->my_modem);
    } else {
        modem_listen(this_handler->my_modem);
    }
}

void handler_set_mode(struct packet_handler *this_handler, enum send_mode mode);

void handler_set_CW(struct packet_handler *this_handler, uint8_t log2CWmin, uint8_t log2CWmax);

static inline bool handler_is_busy(struct packet_handler *this_handler) {
    return this_handler->is_busy;
}

static inline void handler_set_1_persistent_timeout(struct packet_handler *this_handler, uint32_t ms) {
    this_handler->timeout_ms = ms;
}

static inline enum send_outcome handler_get_last_send_output(struct packet_handler *this_handler) {
    return this_handler->last_send_outcome;
}