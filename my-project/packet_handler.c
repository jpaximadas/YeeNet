#include "packet_handler.h"
#include "modem_hl.h"
#include "address.h"
#include "packet.h"
#include <sys/types.h>
#include <stdbool.h>

#define PACKET_PROCESS_OFFSET_MS 5

void handler_post_tx(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;
	//check if rx cleanup will handle mode change
	if(!(this_handler->wait_for_cleanup)){
		modem_listen(this_handler->my_modem);
	}
	//wait for cleanup flag is cleared by rx cleanup
	return;
}

void handler_post_rx(void * param){
	uint8_t recv_len;
	struct packet_handler *this_handler = (struct packet_handler *) param;
	enum payload_status stat = modem_get_payload(this_handler->my_modem,this_handler->rx_pkt,&recv_len);
	this_handler->my_modem->irq_seen = true;
	switch(stat){
		case PAYLOAD_EMPTY:
			this_handler->wait_for_cleanup = true;
			handler_rx_cleanup(this_handler);
			return;
		break;
		case PAYLOAD_BAD;
			this_handler->wait_for_cleanup = true;
			modem_load_and_transmit(this_handler->my_modem,this_handler->my_nack,sizeof(packet_nack));
			handler_rx_cleanup(this_handler);
			return;
		break;
		default:
		break;
	}
	
	//cast top byte to type
	enum packet_type incoming_type = (enum packet_type) ((uint8_t) this_handler->rx_pkt);

	switch(incoming_type){
		case ACK:
			struct packet_ack * my_pkt = (struct packet_ack *) this_handler->rx_pkt;
			if(this_handler->my_state == LOCKED){ //is TX ongoing
				
				if(my_pkt->dest==0x00 || my_pkt->dest==this_handler->my_addr){ //is the packet addressed to this node
			
					if(this_handler->tx_pkt->type == DATA_ACKED){ //is this node waiting for an ack

						if(this_handler->tx_pkt->dest == my_pkt->src){ //is this ack from the node being TX'ed to
				
							remove_timed_callback(this_handler->my_timed_callback);
							handler_success(this_handler);
						}
					}
				}
			}
		break;

		case NACK:
			struct packet_nack * my_pkt = (struct packet_nack *) this_handler->rx_pkt;
			if(this_handler->my_state == LOCKED){ //is tx ongoing

				if(this_handler->tx_pkt->dest == my_pkt->src){ //is this nack from the node being TX'ed to

					if(this_handler->tx_pkt->type != DATA_ACKED){ //ignore nacks if outgoing packet is acked

						switch(this_handler->my_send_mode){//different behavior based on send mode
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

		case DATA_ACKED:
			if(my_pkt->dest==0x00 || my_pkt->dest == this_handler->my_addr){

				this_handler->my_ack.dest = my_pkt->src;
				this_handler->wait_for_cleanup = true;
				modem_load_and_transmit(this_handler->my_modem,(uint8_t *) this_handler->my_ack,sizeof(struct packet_ack));
				(*(this_handler->pkt_rdy_callback))(this_handler->callback_arg); //execute packet ready callback
			}
		break;

		case DATA_UNACKED:
			if(my_pkt->dest==0x00 || my_pkt->dest == this_handler->my_addr){

				(*(this_handler->pkt_rdy_callback))(this_handler->callback_arg); //execute packet ready callback
			}
		break;

		default:
			this_handler->wait_for_cleanup = true;
			modem_load_and_transmit(this_handler->my_modem,(uint8_t *) this_handler->my_nack,sizeof(struct packet_nack));
		break;
	}

	handler_rx_cleanup(this_handler);
	return;
}

void handler_setup
	(
	struct packet_handler *this_handler, 
	struct modem *_my_modem, 
	struct packet_data *_rx_pkt, 
	void (* _pkt_rdy_callback)(void *), 
	void * _callback_arg,
	enum send_mode _my_send_mode, 
	uint8_t _backoffs_max
	) {
	this_handler->my_addr = local_address_get();
	
	(this_handler->my_ack).type = ACK;
	(this_handler->my_ack).src = this_handler->my_addr;
	(this_handler->my_ack).dest = 0; //this is populated before sending
	
	this_handler->ack_airtime_ms = modem_get_airtime_usec(this_handler->my_modem,sizeof(struct packet_ack))/1000;
	
	(this_handler->my_nack).type = NACK;
	(this_handler->my_nack).src = this_handler->my_addr;
	
	this_handler->nack_airtime_ms = modem_get_airtime_usec(this_handler->my_modem,sizeof(struct packet_nack))/1000;
	
	this_handler->pkt_rdy_callback = _pkt_rdy_callback;
	this_handler->callback_arg= _callback_arg;
	
	//workspace for incoming packets. to be made safe to write by pkt_rdy_callback
	this_handler->rx_pkt = _rx_pkt;
	
	this_handler->my_modem = _my_modem;

	this_handler->my_send_mode = _my_send_mode;
	
	this_handler->my_state = UNLOCKED;
	
	this_handler->last_packet_status = SUCCESS;
	
	this_handler->nack_occurred = false;

	this_handler->tx_snooze = false;

	this_handler->backoffs = 0;	

	this_handler->backoffs_max = _backoffs_max;

	this_handler->wait_for_cleanup = false;
	
	modem_attach_callbacks(this_handler->my_modem,&handler_post_rx,&handler_post_tx,this_handler);
}

bool handler_request_transmit(struct packet_handler *this_handler, struct packet_data *pkt){
	if(this_handler->my_state==LOCKED){ return false;}
	
	if(!modem_is_clear(this_handler->my_modem)){return false;}
	
	//packet_handler is clear for tx

	this_handler->tx_pkt= pkt; //assign tx_pkt pointer to input

	//get and store length
	this_handler->pkt_length = this_handler->tx_pkt->len+PACKET_DATA_OVERHEAD;

	//compute and store packet airtime
	this_handler->pkt_airtime_ms = modem_get_airtime_usec(this_handler->my_modem,this_handler->pkt_length)/1000;
	
	//load the packet
	//this also deafens the modem
	modem_load_payload(this_handler->my_modem,(uint8_t *)pkt,this_handler->pkt_length); 
	
	
	//not always used in every mode but cleared anyway
	this_handler->nack_occurred = false;
	this_handler->backoffs = 0;
	this_handler->tx_snooze = false;
	
	
	uint32_t time_ms = 0;
	switch(this_handler->my_send_mode){
		LAZY:
			time_ms = this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms + PACKET_PROCESS_OFFSET_MS;
			switch(this_handler->tx_pkt->type){
				DATA_ACKED:
					add_timed_callback(time_ms,&handler_failure,this_handler,&(this_handler->my_timed_callback));
				break;
				DATA_UNACKED:
					add_timed_callback(time_ms,&handler_success,this_handler,&(this_handler->my_timed_callback));
				break;
				default:
					return false; //other packet types are not allowed
				break;
			}
		break;
		PERSISTENT:
			this_handler->backoffs = 1;
			time_ms = backoff_rng(this_handler->backoffs)*(this_handler->pkt_airtime_ms) + this_handler->ack_airtime_ms + PACKET_PROCESS_OFFSET_MS;
			add_timed_callback(time_ms,&handler_backoff_retransmit,this_handler,&(this_handler->my_timed_callback));

		break;
	}

	modem_transmit(this_handler->my_modem);

}

void handler_failure(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;
	this_handler->last_packet_status = SUCCESS;
	this_handler->my_state = UNLOCKED;
}

void handler_success(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;
	remove_timed_callback(this_handler->my_timed_callback);
	this_handler->last_packet_status = SUCCESS;
	this_handler->my_state = UNLOCKED;
}

void handler_backoff_retransmit(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;


	//If the packet is unacked and no nacks have occurred, clear the nack flag and return
	if( ((struct packet_data *)(this_handler->tx_pkt) )->type == DATA_UNACKED){
		if(this_handler->nack_occurred){
			this_handler->nack_occurred = false;
		} else {
			handler_success(this_handler);
			return;
		}
	}

	if(this_handler->backoffs >= this_handler->backoffs_max){
		handler_failure(this_handler);
		return;
	}

	if(!modem_is_clear(this_handler->my_modem)) {
			//snooze the tx packet_handler side
			this_handler->tx_snooze = true;
			return;
	}
	
	modem_load_payload(this_handler->my_modem,this_handler->tx_pkt,this_handler->pkt_length);

	this_handler->backoffs++;
	uint32_t time_ms = backoff_rng(this_handler->backoffs) * (this_handler->pkt_airtime_ms) + this_handler->ack_airtime_ms + PACKET_PROCESS_OFFSET_MS;

	add_timed_callback(time_ms,&handler_backoff_retransmit,this_handler,&(this_handler->my_timed_callback));

	modem_transmit(this_handler->my_modem);


}


uint32_t backoff_rng(uint32_t bits){
	uint32_t mask = 0xffffffff;
	return rand_32() & mask>>(32-bits);
}
