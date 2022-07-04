#include "address.h"
#include "callback_timer.h"
#include "host_link.h"
#include "modem_hl.h"
#include "modem_ll.h"
#include "packet.h"
#include "packet_handler.h"
#include "payload_buffer.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <uart.h>
#include <util.h>

#define PACKET_PROCESS_OFFSET_MS 1

struct packet_handler handler0;

void handler_post_tx(void *param);

void handler_post_tx(void *param) {
    struct packet_handler *this_handler = (struct packet_handler *)param;
    modem_listen(this_handler->my_modem);
}

void handler_post_rx(void *param);

void handler_terminate(struct packet_handler *this_handler, enum send_outcome outcome) {
    remove_timed_callback(this_handler->my_timed_callback);
    this_handler->last_send_outcome = outcome;
    this_handler->is_busy = false;
}

// should have higher priority than SysTick
void handler_post_rx(void *param) {
    uint8_t recv_len;

    struct packet_handler *this_handler = (struct packet_handler *)param;
    struct payload_record *rec = this_handler->rx_pkt;

    // check for null pointer here
    if (rec == NULL) {
        handler_rx_cleanup(this_handler, false);
        return;
    }

    enum payload_status stat = modem_get_payload(this_handler->my_modem, rec->contents.raw, &(rec->len));

    this_handler->my_modem->irq_seen = true;

    switch (stat) {
        default:
        case PAYLOAD_EMPTY:
            handler_rx_cleanup(this_handler, false);
            return;
            break;
        case PAYLOAD_BAD:
            modem_load_payload(this_handler->my_modem, (uint8_t *)&(this_handler->my_nack),
                               sizeof(struct packet_nack));
            handler_rx_cleanup(this_handler, true);
            return;
            break;
        case PAYLOAD_GOOD:
            break;
    }

    uint8_t tx_on_cleanup = false;

    // read the type of the packet
    enum packet_type incoming_type = rec->contents.packet.type;

    switch (incoming_type) {
        case ACK: {
            struct packet_ack *cur_ack = (struct packet_ack *)&(rec->contents.packet);
            if (this_handler->is_busy) {  // is TX ongoing
                if (cur_ack->dest == 0x00 ||
                    cur_ack->dest == this_handler->my_addr) {        // is the packet addressed to this node
                    if (this_handler->tx_pkt->type == DATA_ACKED) {  // is this node waiting for an ack
                        if (this_handler->tx_pkt->contents.packet.dest ==
                            cur_ack->src) {  // is this ack from the node being TX'ed to

                            handler_terminate(this_handler, SUCCESS);
                        }
                    }
                }
            }
            break;
        }
        case BAK:  // implement special failure mode later
        case NAK: {
            struct packet_nak *cur_nak = (struct packet_nack *)&(rec->contents.packet);
            if (this_handler->is_busy) {  // is tx ongoing

                if (this_handler->tx_pkt->contents.packet.dest ==
                    cur_nak->src) {  // is this nack from the node being TX'ed to

                    if (this_handler->tx_pkt->contents.packet.type !=
                        DATA_ACKED) {                       // ignore nacks if outgoing packet is acked
                        this_handler->nak_occurred = true;  // let the send ISR deal with it
                        break;
                    }
                }
            }
            break;
        }

        case DATA_ACKED: {
            if (rec->contents.packet.dest == 0x00 || rec->contents.packet.dest == this_handler->my_addr) {
                /*
                if(handicap_counter<2){
                        fprintf(fp_uart,"HANDICAP\r\n");
                        handicap_counter++;
                        handler_rx_cleanup(this_handler,false);
                        return;
                }
                handicap_counter = 0;
                */
                this_handler->my_ack.dest = rec->contents.packet.src;
                tx_on_cleanup = true;

                rec = (*(this_handler->pkt_rdy_callback))(
                    this_handler->callback_arg);  // execute packet ready callback
                if (rec == NULL) {
                    modem_load_payload(this_handler->my_modem, (uint8_t *)&(this_handler->my_ack),
                                       sizeof(struct packet_bak));
                } else {
                    this_handler->rx_pkt = rec;
                    modem_load_payload(this_handler->my_modem, (uint8_t *)&(this_handler->my_ack),
                                       sizeof(struct packet_ack));
                }
            }

            break;
        }

        case DATA_UNACKED: {
            if (rec->contents.packet.dest == 0x00 || rec->contents.packet.dest == this_handler->my_addr) {
                rec = (*(this_handler->pkt_rdy_callback))(
                    this_handler->callback_arg);  // execute packet ready callback
                if (rec == NULL) {
                    modem_load_payload(this_handler->my_modem, (uint8_t *)&(this_handler->my_ack),
                                       sizeof(struct packet_bak));
                    tx_on_cleanup = true;
                } else {
                    this_handler->rx_pkt = rec;
                }
            }
            break;
        }

        default: {
            // fprintf(fp_uart, "[DEBUG] invalid packet type\r\n");

            break;
        }
    }

    handler_rx_cleanup(this_handler, tx_on_cleanup);
    return;
}

uint32_t backoff_rng(uint8_t bits);

uint32_t backoff_rng(uint8_t bits) {
    uint32_t mask = 0xffffffff;
    return rand_32() & mask >> (32 - bits);
}

void handler_CSMA_CD_1_persistent(struct packet_handler *this_handler) {
    if (!modem_is_clear(this_handler->my_modem)) {
        add_timed_callback(1, this_handler->send_function, this_handler, &(this_handler->my_timed_callback));
        return;
    }
    modem_load_payload(this_handler->my_modem, this_handler->tx_pkt->contents.raw, this_handler->tx_pkt->len);

    uint32_t time_ms;
    switch (this_handler->tx_pkt->contents.packet.type) {
        case DATA_ACKED:
            time_ms = (1 + backoff_rng(this_handler->log2CWcur)) *
                      (this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms +
                       this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
        case DATA_UNACKED:  // this assumes a NAK will immediately arrive after transmission which may not
                            // always be true
            time_ms = (1 + backoff_rng(this_handler->log2CWcur)) *
                      (this_handler->pkt_airtime_ms + this_handler->nak_airtime_ms +
                       this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
    }
    this_handler->log2CWcur++;
    add_timed_callback(time_ms, this_handler->send_function, this_handler,
                       &(this_handler->my_timed_callback));
    modem_transmit(this_handler->my_modem);

    // Failure if out of new contention window values
    if (this_handler->log2CWnext > this_handler->log2CWmax) {
        this_handler->is_busy = false;
        this_handler->last_send_outcome = FAILED_MAX_BACKOFFS_REACHED;
        return;
    }
    // Success if a NAK didn't come
    if (this_handler->nak_occurred && this_handler->tx_pkt->contents.packet.type == DATA_UNACKED) {
        this_handler->is_busy = false;
        this_handler->last_send_outcome = SUCCESS;
        return;
    }

    if (modem_is_clear(this_handler->my_modem)) {  // modem is clear, try sending

    } else {  // spam the modem, this is where IFS stuff might go and timeout tracking
    }
}

void handler_CSMA_CD_non_persistent(struct packet_handler *this_handler) {
    // Failure if out of new contention window values
    if (this_handler->log2CWnext > this_handler->log2CWmax) {
        this_handler->is_busy = false;
        this_handler->last_send_outcome = FAILED_MAX_BACKOFFS_REACHED;
        return;
    }
    // Success if a NAK didn't come for unacked packet
    if (this_handler->nak_occurred && this_handler->tx_pkt->contents.packet.type == DATA_UNACKED) {
        this_handler->is_busy = false;
        this_handler->last_send_outcome = SUCCESS;
        return;
    }
    uint32_t time_ms;
    switch (this_handler->tx_pkt->contents.packet.type) {
        case DATA_ACKED:
            time_ms = (1 + backoff_rng(this_handler->log2CWnext)) *
                      (this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms +
                       this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
        case DATA_UNACKED:  // this assumes a NAK will immediately arrive after transmission which may not
                            // always be true
            time_ms = (1 + backoff_rng(this_handler->log2CWnext)) *
                      (this_handler->pkt_airtime_ms + this_handler->nak_airtime_ms +
                       this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
    }
    this_handler->log2CWnext++;
    if (modem_is_clear(this_handler->my_modem)) {  // modem is clear, try sending
        modem_load_payload(this_handler->my_modem, this_handler->tx_pkt->contents.raw,
                           this_handler->tx_pkt->len);
        add_timed_callback(time_ms, this_handler->send_function, this_handler,
                           &(this_handler->my_timed_callback));
        modem_transmit(this_handler->my_modem);
    } else {
        add_timed_callback(time_ms, this_handler->send_function, this_handler,
                           &(this_handler->my_timed_callback));
    }
}

void handler_set_mode(struct packet_handler *this_handler, enum send_mode mode) {
    switch (mode) {
        case CSMA_CD_1_PERSISTENT:
            this_handler->send_function = &handler_CSMA_CD_1_persistent;
        case CSMA_CD_NON_PERSISTENT:
        default:
            this_handler->send_function = &handler_CSMA_CD_non_persistent;
    }
    return;
}

bool handler_request_transmit(struct packet_handler *this_handler, struct payload_record *pkt) {
    if (this_handler->is_busy) {
        return false;
    }
    this_handler->is_busy = true;
    // reset state variables;
    this_handler->tx_pkt = pkt;                        // assign tx_pkt pointer to input
    pkt->contents.packet.src = this_handler->my_addr;  // force the source address
    this_handler->pkt_length = this_handler->tx_pkt->len + PACKET_DATA_OVERHEAD;

    // compute and store packet airtime
    this_handler->pkt_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, this_handler->pkt_length) / 1000 + 1;
    // fprintf(fp_uart, "estimated packet airtime: %li\r\n", this_handler->pkt_airtime_ms);

    // not always used in every mode but cleared anyway
    this_handler->nak_occurred = false;
    this_handler->log2CWnext = this_handler->log2CWmin;
    this_handler->last_send_outcome = STALE;

    (*(this_handler->send_function))(this_handler);
    return true;
}

void handler_setup(struct packet_handler *this_handler,
                   struct modem *_my_modem,
                   bool (*_pkt_rdy_callback)(void *),
                   void *_callback_arg) {
    this_handler->my_addr = local_address_get();

    (this_handler->my_ack).type = ACK;
    (this_handler->my_ack).src = this_handler->my_addr;
    this_handler->ack_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, sizeof(struct packet_ack)) / 1000 + 1;

    (this_handler->my_nak).type = NAK;
    (this_handler->my_nak).src = this_handler->my_addr;
    this_handler->nak_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, sizeof(struct packet_nak)) / 1000 + 1;

    (this_handler->my_bak).type = BAK;
    (this_handler->my_bak).src = this_handler->my_addr;
    this_handler->bak_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, sizeof(struct packet_bak)) / 1000 + 1;

    this_handler->my_modem = _my_modem;
    this_handler->pkt_rdy_callback = _pkt_rdy_callback;
    this_handler->callback_arg = _callback_arg;

    // workspace for incoming packets
    this_handler->rx_pkt = payload_record_alloc();

    // set default values
    this_handler->nak_occurred = false;
    this_handler->log2CWmin = 1;
    this_handler->log2CWmax = 3;
    this_handler->is_busy = false;
    this_handler->my_send_mode = &handler_CSMA_CD_1_persistent;
    this_handler->last_send_outcome = STALE;
    this_handler->timeout_ms = modem_get_airtime_usec(this_handler->my_modem, MAX_PAYLOAD_LENGTH) / 1000 + 1;

    modem_attach_callbacks(this_handler->my_modem, &handler_post_rx, &handler_post_tx, this_handler);

    modem_listen(this_handler->my_modem);
}