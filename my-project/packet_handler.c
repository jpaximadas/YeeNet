#include "packet_handler.h"
#include "modem_hl.h"
#incldue "address.h"
#include <sys/types.h>
#include <stdbool.h>

#define PACKET_PROCESS_OFFSET_MS 5

void handler_setup(struct packet_handler *this_handler, struct modem *_my_modem, struct packet_data *_rx_pkt, void (* _pkt_rdy_callback)(struct packet_handler *), enum send_mode _my_send_mode){
	this_handler->my_addr = local_address_get();
	
	this_handler->my_ack->type = ACK;
	this_handler->my_ack->src = this_handler->my_addr;
	this_handler->my_ack->desk = 0; //this is populated before sending
	
	this_hander->ack_airtime = get_airtime(this_handler->my_modem,sizeof(struct packet_ack));
	
	this_handler->my_ack->type = NACK;
	this_handler->my_ack->src = this_handler->my_addr;
	
	this_hander->ack_airtime = get_airtime(this_handler->my_modem,sizeof(struct packet_nack));
	
	this_handler->pkt_rdy_callback = _pkt_ready_callback;
	
	//workspace for incoming packets. to be made safe to write by pkt_rdy_callback
	this_handler->rx_pkt = _rx_pkt;
	
	this_handler->my_modem = _my_modem;

	this_handler->my_send_mode = _my_send_mode;
	
	this_handler->my_state = UNLOCKED;
	
	this_hanlder->last_packet_status = SUCCESS;
	
	nack_occurred = false;

	uint8_t errors = 0;	
	
}

bool handler_request_transmit(struct packet_handler *this_handler, struct packet_data *pkt){
	if(this_handler->my_state==LOCKED){ return false;}
	
	if(!modem_is_clear(this_handler->my_modem)){return false;}
	
	this_handler->airtime = get_airtime(this_handler->my_modem,pkt->length+PACKET_DATA_OVERHEAD);
	
	modem_load_payload(this_handler->my_modem,(uint8_t *)pkt,pkt->length+PACKET_DATA_OVERHEAD); //this deafens the modem
	this_handler->tx_pkt= pkt;
	
	//not always used in every mode but cleared anyway
	this_handler->nack_occurred = false;
	this_handler->errors = false;
	
	
	
	switch(this_handler->my_send_mode){
		LAZY:
			switch(this_handler->tx_pkt->type){
				DATA_ACKED:
					
				break;
				DATA_UNACKED:
				break;
			}
		break;
		PERSISTENT:
		break;
	}
}

void handler_failure(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;
	this_handler->last_packet_status = SUCCESS;
	this_handler->my_state = UNLOCKED;
}

void handler_success(void * param){
	struct packet_handler *this_handler = (struct packet_handler *) param;
	remove_timed_callback(this_handler->my_callback);
	this_handler->last_packet_status = SUCCESS;
	this_handler->my_state = UNLOCKED;
}
