#define DEBUG

#include "address.h"
#include "callback_timer.h"
#include "modem_hl.h"
#include "packet.h"
#include "packet_handler.h"
#include "platform/platform.h"
#include "uart.h"
#include "util.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <stdbool.h>
#include <stdlib.h>

// debug
#include "modem_ll.h"
#include "sx127x.h"

// lora0 is the default modem defined in the platform pinout table
static struct modem_hw lora0_hw;
static struct modem lora0;

// packet_handler layer tester
#if 1

static struct packet_handler lora0_handler;
static struct packet_data incoming_packet;
static struct packet_data outgoing_packet;

bool pkt_avail = false;

void capture_packet(void *param) {
    pkt_avail = true;
}

int main(void) {
    // Setup peripherals
    platform_clocks_init();
    platform_usart_init();
    platform_gpio_init();
    platform_spi_init();

    // Obtain UART FILE*
    fp_uart = uart_setup(platform_pinout.p_usart);

    fprintf(fp_uart, "start setup...\r\n");

    local_address_setup();
    fprintf(fp_uart, "Local address: %x\r\n", local_address_get());

    if (local_address_get() != 1 && local_address_get() != 2 && local_address_get() != 3) {
        fprintf(fp_uart, "Address must be 1, 2, or 3. Halting execution");
        for (;;)
            ;
    }

    outgoing_packet.src = local_address_get();
    switch (local_address_get()) {
        case 1:
            outgoing_packet.dest = 2;
            outgoing_packet.type = DATA_ACKED;
            break;
        case 2:
            outgoing_packet.dest = 1;
            outgoing_packet.type = DATA_ACKED;
            break;
        case 3:
            outgoing_packet.dest = 0;
            outgoing_packet.type = DATA_UNACKED;
            break;
        default:
            outgoing_packet.dest = 0;
            outgoing_packet.type = DATA_UNACKED;
            break;
    }

    callback_timer_setup();

    // Setup lora0 according to the platform-defined pinout
    lora0_hw.spi_interface = platform_pinout.p_spi, lora0_hw.rst = platform_pinout.modem_rst,
    lora0_hw.ss = platform_pinout.modem_ss, lora0_hw.mosi = platform_pinout.modem_mosi,
    lora0_hw.miso = platform_pinout.modem_miso, lora0_hw.sck = platform_pinout.modem_sck,
    lora0_hw.irq = platform_pinout.modem_irq,

    modem_setup(&lora0, &lora0_hw);  // this needs to occur first
    handler_setup(&lora0_handler, &lora0, &incoming_packet, &capture_packet, &lora0_handler, PERSISTENT, 4);

    fprintf(fp_uart, "setup complete\r\n");

    uint8_t send_len;
    for (;;) {
        if (uart_available(platform_pinout.p_usart)) {
            for (int i = 0; i < MAX_PAYLOAD_LENGTH - PACKET_DATA_OVERHEAD; i++)
                outgoing_packet.data[i] = 0;
            send_len = uart_read_until(platform_pinout.p_usart, outgoing_packet.data,
                                       MAX_PAYLOAD_LENGTH - PACKET_DATA_OVERHEAD, '\r') -
                       1;
            outgoing_packet.len = send_len;

            if (lora0_handler.my_state == UNLOCKED &&
                handler_request_transmit(&lora0_handler, &outgoing_packet)) {
                fprintf(fp_uart, "packet away\r\n");
            } else {
                fprintf(fp_uart, "handler busy, try again\r\n");
            }

            while (lora0_handler.my_state == LOCKED) {
                start_timer(1);
                // fprintf(fp_uart,"waiting for ack...\r\n");
                // delay_nops(1000000);
            }

            fprintf(fp_uart, "Locked->Unlocked: %lu\r\n", stop_timer(1));

            if (lora0_handler.last_packet_status == SUCCESS) {
                fprintf(fp_uart, "received ack; exchange successful!\r\n");

            } else {
                fprintf(fp_uart, "ack never came; exchange failed\r\n");
            }
        }

        if (pkt_avail) {
            fprintf(fp_uart, "got packet | ");
            fprintf(fp_uart, "source: %x destination: %x \r\n", incoming_packet.src, incoming_packet.dest);
            fprintf(fp_uart, "length: %x\r\n",
                    incoming_packet.len);  // length can make the loop exceed the size of the struct, fix when
                                           // running for real
            fprintf(fp_uart, "contents: ");
            for (int i = 0; i < incoming_packet.len; i++) {
                fprintf(fp_uart, "%c", (char)incoming_packet.data[i]);
            }
            fprintf(fp_uart, "\r\n");
            fprintf(fp_uart, "RSSI: %ld dB\r\n", get_last_payload_rssi(&lora0));
            double snr = get_last_payload_snr(&lora0);
            int32_t snr_floored = (int32_t)snr;
            int32_t snr_decimals = abs((int32_t)((snr - ((double)snr_floored)) * 100.0));
            fprintf(fp_uart, "SNR: %ld.%ld dB\r\n", snr_floored, snr_decimals);
            pkt_avail = false;
        }
    }
}
#endif

#if 0
//modem layer tester

uint8_t buf[255];

static void my_rx(void * param){
	//fprintf(fp_uart,"in rx isr\r\n");
	struct modem * this_modem = (struct modem *)param;
	uint8_t recv_len;
	for(int i = 0; i<255;i++) buf[i] = 0;
	enum payload_status msg_stat = modem_get_payload(&lora0,buf,&recv_len);
	this_modem->irq_seen = true;
	if (msg_stat == PAYLOAD_GOOD) {
            fprintf(fp_uart,"got message: %s\r\n", buf);
            fprintf(fp_uart,"length: %u\r\n",recv_len);

    } else if(msg_stat == PAYLOAD_BAD) {
            fprintf(fp_uart, "bad packet\r\n");
    } else if(msg_stat == PAYLOAD_EMPTY){
			fprintf(fp_uart,"empty fifo\r\n");
	}

	if(msg_stat != PAYLOAD_EMPTY){
		fprintf(fp_uart,"RSSI: %ld dB\r\n",get_last_payload_rssi(&lora0));
        double snr = get_last_payload_snr(&lora0);
        int32_t snr_floored = (int32_t)snr;
        int32_t snr_decimals = abs((int32_t)((snr - ((double)snr_floored))*100.0));
        fprintf(fp_uart,"SNR: %ld.%ld dB\r\n",snr_floored,snr_decimals);
	}
    modem_listen(&lora0);
}

static void my_tx(void * param){
	struct modem * this_modem = (struct modem *)param;
	this_modem->irq_seen = true;
	//fprintf(fp_uart,"mode at start of my_tx: %x\r\n",lora_read_reg(&lora0,LORA_REG_OP_MODE));
	if(this_modem->irq_data == LORA_MASK_IRQFLAGS_TXDONE){
		fprintf(fp_uart,"transmit was successful\r\n");
		//fprintf(fp_uart,"measured airtime: %lu us \r\n",get_last_airtime(this_modem));
	}else{
		fprintf(fp_uart,"transmit failed!\r\n");
	}
	modem_listen(&lora0);
	//fprintf(fp_uart,"mode at end of my_tx: %x\r\n",lora_read_reg(&lora0,LORA_REG_OP_MODE));
}

uint8_t send_len;

callback_id_t id;

char greeting[3] = "hi";


void hop(void *param){
	char *str = (char *) param;
	fprintf(fp_uart,"%s\r\n",str);
	add_timed_callback(1000, &hop, &greeting, &id);
}

int main(void) {

	clock_setup();
    fp_uart = uart_setup();
    fprintf(fp_uart,"start setup\r\n");
    callback_timer_setup();

    modem_setup(&lora0,&dev_breadboard);
	modem_attach_callbacks(&lora0,&my_tx,&my_rx,&lora0);

	local_address_setup();
	fprintf(fp_uart,"Local address: %x\r\n",local_address_get());

    fprintf(fp_uart,"finished setup!\r\n");

    //hop(&greeting);

    modem_listen(&lora0);


    for(;;){
        //fprintf(fp_uart,"looping\r\n");

        if(uart_available()){

			for(int i = 0; i<255;i++) buf[i] = 0;
			send_len = uart_read_until(platform_usart, buf, sizeof(buf), '\r')-1;

			fprintf(fp_uart,"packet size: %u\r\n", send_len);
			fprintf(fp_uart,"est. airtime: %lu us\r\n", modem_get_airtime_usec(&lora0,send_len));
			if(modem_is_clear(&lora0)){
				modem_load_and_transmit(&lora0,buf,send_len);
			} else {
				fprintf(fp_uart,"modem is busy!\r\n");
			}
		}

		delay_nops(10000);
    }
}
#endif
