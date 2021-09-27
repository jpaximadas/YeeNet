#include "host_link.h"
#include "platform/platform.h"
#include <stdbool.h>
#include "packet_buffer.h"
#include "address.h"

struct packet_record records[PACKET_BUFFER_LENGTH];
struct packet_buf packet_buffer;

int main(void) {
	
	platform_clocks_init();
    platform_gpio_init();
    platform_spi_init();
    local_address_setup();

	packet_buf_init(&packet_buffer,records);
    host_link_init(USART);

    platform_set_indicator(true);
    delay_nops(1000000);
    platform_set_indicator(false);

	for(;;){
		host_link_parse();
	}
}
