#pragma once
#include "packet.h"
#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

#define ALL_FREE_RECORDS 6

union payload {
    struct packet_data packet;
    uint8_t raw[MAX_PAYLOAD_LENGTH];
};

enum payload_type { RAW_GOOD = 0, RAW_BAD, PACKET, OUTGOING_PACKET };

struct payload_record {
    enum payload_type type;
    union payload contents;
    float snr;
    int32_t rssi;
    uint8_t len;
    uint32_t time;
};

struct payload_buffer {
    struct payload_record *records[ALL_FREE_RECORDS];
    uint8_t start;
    uint8_t end;
    bool full;
    uint8_t len;
    uint8_t n_underflow;
    uint8_t n_overflow;
};

extern struct payload_buffer host_link_payload_buffer;

extern struct payload_buffer free_records;

void payload_buffer_init(struct payload_buffer *this_buf);

bool payload_buffer_push(struct payload_buffer *this_buf, struct payload_record *in);

struct payload_record *payload_buffer_pop(struct payload_buffer *this_buf);

uint8_t payload_buffer_capacity(struct payload_buffer *this_buf);

void free_records_init();

struct payload_record *payload_record_alloc();

bool payload_record_free(struct payload_record *in);

uint8_t n_free_records();