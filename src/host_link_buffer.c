#include "host_link.h"
#include "host_link_buffer.h"
#include "payload_buffer.h"

void host_link_buffer_pop(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    uint8_t waiting = payload_buffer_capacity(&host_link_payload_buffer);
    link.iface_write_byte(waiting);
    if (waiting) {
        struct payload_record *rec = payload_buffer_pop(&host_link_payload_buffer);
        link.iface_write_bytes((uint8_t *)&(rec->rssi), 4);
        link.iface_write_bytes((uint8_t *)&(rec->snr), 4);
        link.iface_write_bytes(rec->contents.raw_payload.payload, rec->contents.raw_payload.len);
        payload_record_free(rec);
    }
    return;
}

void host_link_buffer_cap(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    uint8_t cap = payload_buffer_capacity(&host_link_payload_buffer);
    link.iface_write_byte(cap);
    return;
}

void host_link_buffer_get_n_overflow(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    link.iface_write_byte(free_records.n_underflow);
    return;
}

void host_link_buffer_reset_n_overflow(uint8_t *command, uint16_t len) {
    (void)command;
    (void)len;
    free_records.n_underflow = 0;
    return;
}
