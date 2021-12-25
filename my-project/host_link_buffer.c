#include "host_link.h"
#include "host_link_buffer.h"
#include "packet_buffer.h"

void host_link_buffer_pop(uint8_t *command, uint16_t len) {
    uint8_t waiting = packet_buf_get_cap(&packet_buffer);
    link.iface_write_byte(waiting);
    if (waiting) {
        struct packet_record *rec =
            packet_buf_peek(&packet_buffer);  // only peek, this will prevent an interrupt from writing to
                                              // this memory while we work
        link.iface_write_bytes((uint8_t *)&(rec->rssi), 4);
        link.iface_write_bytes((uint8_t *)&(rec->snr), 4);
        link.iface_write_bytes(rec->packet, rec->length);
        packet_buf_pop(
            &packet_buffer);  // the data has been moved to a buffer thru iface_write, free the memory
    }
    return;
}

void host_link_buffer_cap(uint8_t *command, uint16_t len) {
    uint8_t cap = packet_buf_get_cap(&packet_buffer);
    link.iface_write_byte(cap);
    return;
}

void host_link_buffer_get_n_overflow(uint8_t *command, uint16_t len) {
    link.iface_write_bytes(&n_overflow, sizeof(n_overflow));
    return;
}

void host_link_buffer_reset_n_overflow(uint8_t *command, uint16_t len) {
    n_overflow = 0;
    return;
}
