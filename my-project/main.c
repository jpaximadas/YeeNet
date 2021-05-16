#include "host_link.h"
#include "platform/platform.h"
#include <stdbool.h>
#include "packet_buffer.h"

struct packet_record records[PACKET_BUFFER_LENGTH];
struct packet_buf packet_buffer;

int main(void) {
	
	platform_clocks_init();
    platform_gpio_init();

	packet_buf_init(&packet_buffer,records);
    host_link_init(USART);
	for(;;){
		host_link_parse();
	}
}
