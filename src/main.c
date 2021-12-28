#include "address.h"
#include "host_link.h"
#include "payload_buffer.h"
#include "platform/platform.h"
#include <stdbool.h>

int main(void) {
    platform_clocks_init();
    platform_gpio_init();
    platform_spi_init();
    local_address_setup();

    free_records_init();  // initialize packet record pool
    host_link_init(USART);

    platform_set_indicator(true);
    delay_nops(1000000);
    platform_set_indicator(false);

    for (;;) {
        host_link_parse();
    }
}
