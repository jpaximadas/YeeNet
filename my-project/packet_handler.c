#include "packet_handler.h"
#include "modem_hl.h"
#include "address.h"
#include "packet_handler.h"
#include <sys/types.h>
#include <stdbool.h>

#define PACKET_PROCESS_OFFSET_MS 5

void handler_setup(struct packet_handler *this_handler, struct modem *_my_modem, struct packet_data *_rx_pkt, void (* _pkt_rdy_callback)(struct packet_handler *), enum send_mode _my_send_mode){
	this_handler->my_addr = local_address_get();
	
	(this_handler->my_ack).type = ACK;
	(this_handler->my_ack).src = this_handler->my_addr;
	(this_handler->my_ack).dest = 0; //this is populated before sending
	
	this_handler->ack_airtime_ms = modem_get_airtime_usec(this_handler->my_modem,sizeof(struct packet_ack))/1000;
	
	(this_handler->my_nack).type = NACK;
	(this_handler->my_nack).src = this_handler->my_addr;
	
	this_handler->nack_airtime_ms = modem_get_airtime_usec(this_handler->my_modem,sizeof(struct packet_nack))/1000;
	
	this_handler->pkt_rdy_callback = _pkt_rdy_callback;
	
	//workspace for incoming packets. to be made safe to write by pkt_rdy_callback
	this_handler->rx_pkt = _rx_pkt;
	
	this_handler->my_modem = _my_modem;

	this_handler->my_send_mode = _my_send_mode;
	
	this_handler->my_state = UNLOCKED;
	
	this_handler->last_packet_status = SUCCESS;
	
	this_handler->nack_occurred = false;

	uint8_t errors = 0;	
	
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
	this_handler->errors = 0;
	
	
	uint32_t time_ms = 0;
	switch(this_handler->my_send_mode){
		LAZY:
			switch(this_handler->tx_pkt->type){
				DATA_ACKED:
					offset = this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms + PACKET_PROCESS_OFFSET_MS;
					add_timed_callback(time_ms,&handler_failure,this_handler,&(this_handler->my_timed_callback));
				break;
				DATA_UNACKED:
					offset = this_handler->pkt_airtime_ms + this_handler->nack_airtime_ms + PACKET_PROCESS_OFFSET_MS;
					add_timed_callback(time_ms,&handler_success,this_handler,&(this_handler->my_timed_callback));
				break;
			}
		break;
		PERSISTENT:
			time_ms = this_handler->pkt_airtime_ms + this_handler->ack_airtime_ms + PACKET_PROCESS_OFFSET_MS;
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
}


uint32_t backoff_rng(uint32_t bits){
	uint32_t mask = 0xffffffff;
	return rand_32() & mask>>(32-bits);
}