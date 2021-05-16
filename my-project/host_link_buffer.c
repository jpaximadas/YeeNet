#include "host_link_buffer.h"
#include "host_link.h"
#include "packet_buffer.h"

void host_link_buffer_pop(uint8_t *command, uint16_t len){
    struct packet_record *rec = packet_buf_peek(&packet_buffer);
    if(rec == NULL){
        uint8_t empty = 0x00;
        link.iface_write_bytes(&empty,1);
    } else {
        link.iface_write_bytes(&(rec->length),1); //write the length as the first byte instead of 0x00
        link.iface_write_bytes(rec->packet,rec->length); //write the packet into the serial output buffer
        packet_buf_pop(&packet_buffer); //free up the memory
    }
    return;
}

void host_link_buffer_cap(uint8_t *command, uint16_t len){
    uint8_t cap = packet_buf_get_cap(&packet_buffer);
    link.iface_write_bytes(&cap,1);
    return;
}