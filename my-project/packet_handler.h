
#include "modem_hl.h"
#include "address.h"
#include <sys/types.h>
#include <stdbool.h>
#include "callback_timer.h"
#include "packet.h"


enum send_mode{
	LAZY=0,
	PERSISTENT
};

enum handler_state{
	UNLOCKED=0,
	LOCKED
};

enum packet_state{
	SUCCESS=0,
	FAILED
};

struct packet_handler{
	struct modem *my_modem;
	
	uint8_t my_addr;

	enum send_mode my_send_mode;
	enum handler_state my_state;
	enum packet_state last_packet_status;
	
	bool nack_occurred;
	uint8_t backoffs;
	uint8_t backoffs_max;
	bool tx_snooze;
	bool wait_for_cleanup;
	callback_id_t my_timed_callback;
	uint32_t pkt_airtime_ms;
	uint8_t pkt_length;
	
	struct packet_data *tx_pkt;
	struct packet_data *rx_pkt;
	
	void (* pkt_rdy_callback)(void *);
	void * callback_arg;
	
	struct packet_ack my_ack;
	uint32_t ack_airtime_ms;
	struct packet_nack my_nack;
	uint32_t nack_airtime_ms;
	
	
};

void handler_setup
	(
	struct packet_handler *this_handler, 
	struct modem *_my_modem, 
	struct packet_data *_rx_pkt, 
	void (* _pkt_rdy_callback)(void *), 
	void * _callback_arg,
	enum send_mode _my_send_mode, 
	uint8_t _backoffs_max
	);

bool request_transmit(struct packet_handler *this_handler, struct packet_data *pkt);

static inline void handler_rx_cleanup(struct packet_handler *this_handler){
	if(this_handler->tx_snooze){
		handler_backoff_retransmit(this_handler);
		this_handler->tx_snooze = false;
	} 
	this_handler->wait_for_cleanup = false;
	modem_listen(this_handler->my_modem);
}