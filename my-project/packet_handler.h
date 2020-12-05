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
 * Enum for indicating how a packet handler will deal with a reception failure on a destination node.
 */
enum send_mode {
    /**
     * Packet handler will not attempt to retransmit after a detected reception failure at the destination.
     *
     * PACKET_UNACKED: Reception of a NACK from the destination results in FAILED packet handler_state.
     * Absence of a NACK results in SUCCESS packet handler state
     *
     * PACKET_ACKED: NACKS are ignored since they are sometimes emitted when LoRa modems parse noise (mostly
     * in SF6). ACK from destination node results in SUCCESS packet handler state. Absence of am ack results
     * in FAILED packet handler state.
     */
    LAZY = 0,
    /**
     * Packet handler will attempt to retransmit until reception success or number of retransmissions equals
     * packet_handler member "backoffs_max"
     *
     * PACKET_UNACKED: Reception of a NACK from the destination results in retransmission of packet or failure
     * when backoffs_max is reached. Absence of a NACK results in SUCCESS packet handler state
     *
     * PACKET_ACKED: NACKS are ignored since they are sometimes emitted when LoRa modems parse noise (mostly
     * in SF6). ACK from destination node results in SUCCESS packet handler state. Absence of an ack results
     * in successive, delayed retransmissions (exponential backoff).
     */
    PERSISTENT
};

/**
 * Enum for indicating handler TX state.
 */
enum handler_state {
    /**
     * Packet handler is ready to accept a new outgoing packet.
     */
    UNLOCKED = 0,
    /**
     * Packet handler is trying to transmit a packet and cannot accept a new outgoing packet.
     */
    LOCKED
};

/**
 * Enum for expressing the status of the last transmitted packet.
 */
enum packet_state {
    /**
     * The handler concludes that the receiver got the last packet.
     */
    SUCCESS = 0,
    /**
     * The handler concludes that the receiver did not get the last packet.
     */
    FAILED
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

    /** defines how the packet handler deals with reception failures in destination nodes. Enum definition
     * contains more details. */
    enum send_mode my_send_mode;

    /** indicates whether the handler is ready for a new packet */
    enum handler_state my_state;

    /** indicates status of last packet that was sent */
    enum packet_state last_packet_status;

    /** <b>internal variable</b> indicates if NACK was received when attempting the tranmission of a packet */
    bool nack_occurred;

    /** <b>internal variable</b> indicates how many times a retransmission has been attempted in PERSISTENT
     * mode*/
    uint8_t backoffs;

    /** <b>internal variable</b> max number of times a retransmission can occur in PERSISTENT mode */
    uint8_t backoffs_max;

    /** <b>internal variable</b> indicates whether a retransmission attempted to start while the modem was
     * busy receiving a packet */
    bool tx_snooze;

    /** <b>internal variable</b> ID for adding and removing callback_timer for timeouts and retransmissions */
    callback_id_t my_timed_callback;

    /** <b>internal variable</b> removes need to recompute packet airtime after first transmit */
    uint32_t pkt_airtime_ms;

    /** <b>internal variable</b> indicates packets length of current outgoing packet */
    uint8_t pkt_length;

    /** pointer that is moved to location of current outgoing packet on call to handler_request_transmit */
    struct packet_data *tx_pkt;

    /** pointer to location of where incoming packets should be written to */
    struct packet_data *rx_pkt;

    /** function pointer for callback executed after a data packet is received and processed */
    void (*pkt_rdy_callback)(void *);

    /** void pointer to data meant to be the argument to the callback function */
    void *callback_arg;

    /** <b>internal variable</b> ACK packet with blank destination meant to be populated upon use */
    struct packet_ack my_ack;

    /** <b>internal variable</b>  precomputed ack airtime */
    uint32_t ack_airtime_ms;

    /** <b>internal variable</b> NACK packet for use when the modem reports an error */
    struct packet_nack my_nack;

    /** <b>internal variable</b> precomputed nack airtime */
    uint32_t nack_airtime_ms;
};

/**
 * Set up a packet handler.
 *
 * Preconditions:
 * - modem is set up
 * - address has been determined
 * - modem RNG has been seeded
 *
 * Postconditions:
 * - packet handler is ready to use
 * - modem is listening
 *
 * @param this_handler pointer to handler struct to be set up
 * @param my_modem modem to be used by the handler
 * @param _rx_pkt location for incoming packets to be written
 * @param _pkt_rdy_callback function pointer to be dereferenced after an incoming packet is written to
 * _rx_packet
 * @param _callback_arg argument for the pkt_rdy function
 * @param _my_send_mode send mode of the packet handler
 * @param _backoffs_max maximum number of backoffs before failure is reported in PERSISTENT mode
 */
void handler_setup(struct packet_handler *this_handler,
                   struct modem *_my_modem,
                   struct packet_data *_rx_pkt,
                   void (*_pkt_rdy_callback)(void *),
                   void *_callback_arg,
                   enum send_mode _my_send_mode,
                   uint8_t _backoffs_max);

/**
 * Attempt to transmit a packet.
 *
 * Packet handler will check two things before transmitting:
 *  - Whether or not the handler is LOCKED
 *  - Whether or not the modem is busy
 *
 * If both checks pass, the packet handler is reset for a new packet, the packet is transmitted, and true is
 * returned If either check fails, false is returned. packet_handler->my_state can be polled to avoid
 * performing a function call however even if the handler state is UNLOCKED this does not preclude the
 * possibility that the modem will be busy.
 *
 * @param this_handler pointer to handler struct
 * @param pkt pointer to packet ot be transmitted
 * @return whether or not the packet handler is starting the transmission
 */
bool handler_request_transmit(struct packet_handler *this_handler, struct packet_data *pkt);

/**
 * Change location a new packet is written to
 *
 * @param this_handler pointer to handler struct
 * @param new_location pointer to packet_data struct
 */
void set_rx_pkt_pointer(struct packet_handler *this_handler, struct packet_data *new_location);

static void handler_backoff_retransmit(void *param);

static inline void handler_rx_cleanup(struct packet_handler *this_handler, bool tx_on_cleanup) {
    if (tx_on_cleanup) {
        modem_transmit(this_handler->my_modem);

    } else {
        if (this_handler->tx_snooze) {  // this part probably doesn't work
            handler_backoff_retransmit(this_handler);
            this_handler->tx_snooze = false;
        }
        modem_listen(this_handler->my_modem);
    }
}