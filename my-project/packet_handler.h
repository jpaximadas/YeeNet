#include "packet_handler.h"
#include "modem_hl.h"
#incldue "address.h"
#include <sys/types.h>
#include <stdbool.h>


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
	
	enum send_mode my_send_mode;
	enum handler_state my_state;
	enum packet_state last_packet_status;
	
	bool nack_occurred;
	uint8_t errors;
	callback_id_t my_timed_callback;
	uint32_t airtime;
	
	struct packet_data *tx_pkt;
	struct packet_data *rx_pkt;
	
	void (* pkt_rdy_callback)(struct packet_handler *);
	
	struct packet_ack my_ack;
	uint32_t ack_airtime;
	struct packet_nack my_nack;
	uint32_t nack_airtime;
	
	
}

void handler_setup

bool request_transmit(struct packet_handler *this_handler, struct packet_data *pkt);
