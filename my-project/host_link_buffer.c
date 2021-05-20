#include "host_link_buffer.h"
#include "host_link.h"
#include "packet_buffer.h"

/**
 * first byte is number of packets waiting when called
 */
void host_link_buffer_pop(uint8_t *command, uint16_t len){
    uint8_t waiting = packet_buf_get_cap(&packet_buffer);
    link.iface_write_byte(waiting);
    if(waiting){
        struct packet_record *rec = packet_buf_peek(&packet_buffer);
        link.iface_write_bytes((uint8_t*) &(rec->rssi), 4);
        link.iface_write_bytes((uint8_t*) &(rec->snr), 4);
        link.iface_write_bytes(rec->packet,rec->length);
        packet_buf_pop(&packet_buffer); //free up the memory
    }
    return;
}

/**
 * send back packets in buffer
 */
void host_link_buffer_cap(uint8_t *command, uint16_t len){
    uint8_t cap = packet_buf_get_cap(&packet_buffer);
    link.iface_write_byte(cap);
    return;
}