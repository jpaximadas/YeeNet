#include "host_link_modem.h"
#include "modem_hl.h"
#include "platform/platform.h"
#include "packet_buffer.h"
#include "modem_ll_config.h"

struct modem host_link_modem;

struct packet_buf host_link_packet_buf;

void host_link_modem_packet_capture(void *param){
    struct packet_buf *buf = (struct packet_buf *)param;
    struct packet_record *record = packet_buf_push(buf);
    modem_get_payload(&host_link_modem,record->packet,&(record->length));
    record->rssi = modem_get_last_payload_rssi(&host_link_modem);
    record->snr = modem_get_last_payload_snr(&host_link_modem);
}

volatile bool host_link_tx_in_progress = false;
void host_link_modem_tx_done(void *param){
    host_link_tx_in_progress = false;
}

//TODO add modulation config
void host_link_modem_setup(uint8_t *command, uint16_t len){
    modem_setup(&host_link_modem, platform_pinout.p_spi, platform_pinout.modem_ss, platform_pinout.modem_rst);
    modem_attach_callbacks(&host_link_modem,&host_link_modem_packet_capture,&host_link_modem_tx_done, &host_link_packet_buf);
    return;
}

void host_link_modem_listen(uint8_t *command, uint16_t len){
    modem_listen(&host_link_modem);
}

void host_link_modem_load_payload(uint8_t *command, uint16_t len){
    if(len==0){
        link.iface_write_byte(0x01);
        return;
    }

    uint16_t max_length;
    if(payload_length_is_fixed(&host_link_modem)){
        max_length = (host_link_modem.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if(fits){
        modem_load_payload(&host_link_modem,command,len);
    }else{
        link.iface_write_byte(0x02);
        return;
    }

    link.iface_write_byte(0x00);
    return;
}

/**
 *  0 is OK, 1 is empty packet, 2 is too long
 */
void host_link_modem_load_and_transmit(uint8_t *command, uint16_t len){
    if(len==0){
        link.iface_write_byte(0x01);
        return;
    }
    
    uint16_t max_length;
    if(payload_length_is_fixed(&host_link_modem)){
        max_length = (host_link_modem.modulation)->payload_length;
    } else {
        max_length = MAX_PAYLOAD_LENGTH;
    }

    bool fits = len <= max_length;
    if(fits){
        modem_load_payload(&host_link_modem,command,len);
    }else{
        link.iface_write_byte(0x02);
        return;
    }

    host_link_tx_in_progress = true;
    modem_transmit(&host_link_modem);
    while(host_link_tx_in_progress);
    link.iface_write_byte(0x00);
    return;
}

void host_link_modem_transmit(uint8_t *command, uint16_t len){
    host_link_tx_in_progress = true;
    modem_transmit(&host_link_modem);
    while(host_link_tx_in_progress);
    return;
}

void host_link_modem_standby(uint8_t *command, uint16_t len){
    modem_standby(&host_link_modem);
    return;
}

void host_link_modem_is_clear(uint8_t *command, uint16_t len){
    bool status = modem_is_clear(&host_link_modem);
    link.iface_write_byte(status);
    return;
}

void host_link_modem_get_last_payload_rssi(uint8_t *command, uint16_t len){
    uint32_t rssi = modem_get_last_payload_rssi(&host_link_modem);
    link.iface_write_bytes((uint8_t *)&rssi, 4);
    return;
}

void host_link_modem_get_last_payload_snr(uint8_t *command, uint16_t len){
    float snr = modem_get_last_payload_snr(&host_link_modem);
    link.iface_write_bytes((uint8_t *)&snr, 4);
    return;
}

/**
 * Takes a one byte command
 * First return byte is status. 0x00 is good, 0x01 is bad.
 */ 
void host_link_modem_get_airtime_usec(uint8_t *command, uint16_t len){
    if (len == 1){
        link.iface_write_byte(0x00); //report a good command
        uint32_t airtime_usec = modem_get_airtime_usec(&host_link_modem, command[0]);
        link.iface_write_bytes((uint8_t *)&airtime_usec,4);
        return;
    } else{
        link.iface_write_byte(0x01); //report a bad command
    }
    
}