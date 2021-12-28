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

void handler_failure(void *param);

void handler_failure(void *param) {
    struct packet_handler *this_handler = (struct packet_handler *)param;
    this_handler->last_packet_status = FAILED;
    this_handler->my_state = UNLOCKED;
}

void handler_success(void *param);

void handler_success(void *param) {
    struct packet_handler *this_handler = (struct packet_handler *)param;
    remove_timed_callback(this_handler->my_timed_callback);
    this_handler->last_packet_status = SUCCESS;
    this_handler->my_state = UNLOCKED;
}

void handler_post_tx(void *param);

void handler_post_tx(void *param) {
    struct packet_handler *this_handler = (struct packet_handler *)param;
    modem_listen(this_handler->my_modem);
    // fprintf(fp_uart,"mode reg %x\r\n",lora_read_reg(this_handler->my_modem, LORA_REG_OP_MODE));
}

uint8_t handicap_counter = 0;

void handler_post_rx(void *param);

void handler_post_rx(void *param) {
    uint8_t recv_len;

    struct packet_handler *this_handler = (struct packet_handler *)param;
    struct payload_record *rec = this_handler->rx_pkt;
    enum payload_status stat = modem_get_payload(this_handler->my_modem, rec->contents.raw, &(rec->len));

    this_handler->my_modem->irq_seen = true;

    switch (stat) {
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
        default:
            break;
    }

    uint8_t tx_on_cleanup = false;

    // read the type of the packet
    enum packet_type incoming_type = rec->contents.packet.type;

    switch (incoming_type) {
        case ACK: {
            struct packet_ack *cur_ack = (struct packet_ack *)&(rec->contents.packet);
            if (this_handler->my_state == LOCKED) {  // is TX ongoing
                if (cur_ack->dest == 0x00 ||
                    cur_ack->dest == this_handler->my_addr) {        // is the packet addressed to this node
                    if (this_handler->tx_pkt->type == DATA_ACKED) {  // is this node waiting for an ack
                        if (this_handler->tx_pkt->dest ==
                            cur_ack->src) {  // is this ack from the node being TX'ed to

                            remove_timed_callback(this_handler->my_timed_callback);
                            handler_success(this_handler);
                        }
                    }
                }
            }
            break;
        }

        case NACK: {
            struct packet_nack *cur_nack = (struct packet_nack *)&(rec->contents.packet);
            if (this_handler->my_state == LOCKED) {  // is tx ongoing

                if (this_handler->tx_pkt->dest ==
                    cur_nack->src) {  // is this nack from the node being TX'ed to

                    if (this_handler->tx_pkt->type !=
                        DATA_ACKED) {  // ignore nacks if outgoing packet is acked

                        switch (this_handler->my_send_mode) {  // different behavior based on send mode
                            case LAZY:
                                remove_timed_callback(this_handler->my_timed_callback);
                                handler_failure(this_handler);
                                break;
                            case PERSISTENT:
                                this_handler->nack_occurred = true;
                                break;
                        }
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
                modem_load_payload(this_handler->my_modem, (uint8_t *)&(this_handler->my_ack),
                                   sizeof(struct packet_ack));
                (*(this_handler->pkt_rdy_callback))(
                    this_handler->callback_arg);  // execute packet ready callback
            }

            break;
        }

        case DATA_UNACKED: {
            if (rec->contents.packet.dest == 0x00 || rec->contents.packet.dest == this_handler->my_addr) {
                (*(this_handler->pkt_rdy_callback))(
                    this_handler->callback_arg);  // execute packet ready callback
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

void handler_setup(struct packet_handler *this_handler,
                   struct modem *_my_modem,
                   struct payload_record *_rx_pkt,
                   void (*_pkt_rdy_callback)(void *),
                   void *_callback_arg,
                   enum send_mode _my_send_mode,
                   uint8_t _backoffs_max) {
    this_handler->my_addr = local_address_get();

    (this_handler->my_ack).type = ACK;
    (this_handler->my_ack).src = this_handler->my_addr;
    (this_handler->my_ack).dest = 0;  // this is populated before sending

    this_handler->my_modem = _my_modem;

    this_handler->ack_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, sizeof(struct packet_ack)) / 1000 + 1;
    // fprintf(fp_uart, "estimated ack airtime: %li\r\n", this_handler->ack_airtime_ms);

    (this_handler->my_nack).type = NACK;
    (this_handler->my_nack).src = this_handler->my_addr;

    this_handler->nack_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, sizeof(struct packet_nack)) / 1000 + 1;
    // fprintf(fp_uart, "estimated nack airtime: %li\r\n", this_handler->nack_airtime_ms);

    this_handler->pkt_rdy_callback = _pkt_rdy_callback;
    this_handler->callback_arg = _callback_arg;

    // workspace for incoming packets. to be made safe to write by pkt_rdy_callback
    this_handler->rx_pkt = _rx_pkt;

    this_handler->my_send_mode = _my_send_mode;

    this_handler->my_state = UNLOCKED;

    this_handler->last_packet_status = SUCCESS;

    this_handler->nack_occurred = false;

    this_handler->tx_snooze = false;

    this_handler->backoffs = 0;

    this_handler->backoffs_max = _backoffs_max;

    modem_attach_callbacks(this_handler->my_modem, &handler_post_rx, &handler_post_tx, this_handler);

    modem_listen(this_handler->my_modem);
}

void handler_set_rx_pkt_pointer(struct packet_handler *this_handler, struct packet_data *new_location) {
    this_handler->rx_pkt = new_location;
}

uint32_t backoff_rng(uint8_t bits);

uint32_t backoff_rng(uint8_t bits) {
    uint32_t mask = 0xffffffff;
    return rand_32() & mask >> (32 - bits);
}

bool handler_request_transmit(struct packet_handler *this_handler, struct packet_data *pkt) {
    if (this_handler->my_state == LOCKED) {
        return false;
    }

    if (!modem_is_clear(this_handler->my_modem)) {
        return false;
    }

    // packet_handler is clear for tx

    this_handler->my_state = LOCKED;

    this_handler->tx_pkt = pkt;  // assign tx_pkt pointer to input

    // get and store length
    this_handler->pkt_length = this_handler->tx_pkt->len + PACKET_DATA_OVERHEAD;

    // compute and store packet airtime
    this_handler->pkt_airtime_ms =
        modem_get_airtime_usec(this_handler->my_modem, this_handler->pkt_length) / 1000 + 1;
    // fprintf(fp_uart, "estimated packet airtime: %li\r\n", this_handler->pkt_airtime_ms);

    // load the packet
    // this also deafens the modem
    modem_load_payload(this_handler->my_modem, (uint8_t *)pkt, this_handler->pkt_length);

    // not always used in every mode but cleared anyway
    this_handler->nack_occurred = false;
    this_handler->backoffs = 0;
    this_handler->tx_snooze = false;

    uint32_t time_ms = 0;
    switch (this_handler->my_send_mode) {
        case LAZY:
            time_ms = this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms +
                      this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS;
            // fprintf(fp_uart, "timout set to %lu \r\n", time_ms);
            switch (this_handler->tx_pkt->type) {
                // TODO handle add_timed_callback retval
                case DATA_ACKED:

                    add_timed_callback(time_ms, &handler_failure, this_handler,
                                       &(this_handler->my_timed_callback));
                    break;
                case DATA_UNACKED:
                    add_timed_callback(time_ms, &handler_success, this_handler,
                                       &(this_handler->my_timed_callback));
                    break;
                default:
                    return false;  // other packet types are not allowed
                    break;
            }
            break;
        case PERSISTENT:
            this_handler->backoffs = 1;
            time_ms = (1 + backoff_rng(this_handler->backoffs)) *
                      (this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms +
                       this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
            add_timed_callback(time_ms, &handler_backoff_retransmit, this_handler,
                               &(this_handler->my_timed_callback));

            break;
    }

    modem_transmit(this_handler->my_modem);

    return true;
}

static void handler_backoff_retransmit(void *param) {
    struct packet_handler *this_handler = (struct packet_handler *)param;

    // If the packet is unacked and no nacks have occurred, clear the nack flag and return
    if (((struct packet_data *)(this_handler->tx_pkt))->type == DATA_UNACKED) {
        if (this_handler->nack_occurred) {
            this_handler->nack_occurred = false;
        } else {
            handler_success(this_handler);
            return;
        }
    }

    if (this_handler->backoffs >= this_handler->backoffs_max) {
        handler_failure(this_handler);
        return;
    }

    if (!modem_is_clear(this_handler->my_modem)) {
        // snooze the tx packet_handler side
        this_handler->tx_snooze = true;
        return;
    }

    modem_load_payload(this_handler->my_modem, (uint8_t *)this_handler->tx_pkt, this_handler->pkt_length);

    this_handler->backoffs++;
    uint32_t time_ms = (1 + backoff_rng(this_handler->backoffs)) *
                       (this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms +
                        this_handler->my_modem->extra_time_ms + PACKET_PROCESS_OFFSET_MS);
    // fprintf(fp_uart, "waiting for: %lu\r\n", time_ms);

    add_timed_callback(time_ms, &handler_backoff_retransmit, this_handler,
                       &(this_handler->my_timed_callback));

    modem_transmit(this_handler->my_modem);
}
