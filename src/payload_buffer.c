#include "payload_buffer.h"
#include <stdbool.h>
#include <sys/types.h>

struct payload_record records[ALL_FREE_RECORDS];  // all the records for the system

struct payload_buffer free_records;
struct payload_buffer host_link_payload_buffer;

void payload_buffer_init(struct payload_buffer *this_buf) {
    this_buf->len = ALL_FREE_RECORDS;
    this_buf->full = false;
    this_buf->start = 0;
    this_buf->end = 0;
    this_buf->n_overflow = 0;
    this_buf->n_underflow = 0;
}

// push a record in by reference
bool payload_buffer_push(struct payload_buffer *this_buf, struct payload_record *in) {
    if (this_buf->full) {
        this_buf->n_overflow++;  // TODO replace with saturation of uint8_t
        return false;
    }
    (this_buf->records)[this_buf->end] = in;
    if (this_buf->end == (this_buf->len - 1)) {
        this_buf->end = 0;
    } else {
        this_buf->end++;
    }
    if (this_buf->end == this_buf->start)
        this_buf->full = true;
    return true;
}

struct payload_record *payload_buffer_pop(struct payload_buffer *this_buf) {
    if (!(this_buf->full) && this_buf->start == this_buf->end) {
        this_buf->n_underflow++;
        return (struct payload_record *)NULL;
    }
    this_buf->full = false;

    struct payload_record *retval = (this_buf->records)[this_buf->start];

    if (this_buf->start == (this_buf->len - 1)) {
        this_buf->start = 0;
    } else {
        this_buf->start++;
    }

    return retval;
}

uint8_t payload_buffer_capacity(struct payload_buffer *this_buf) {
    uint8_t size = this_buf->len;

    if (!this_buf->full) {
        if (this_buf->end >= this_buf->start) {
            size = (this_buf->end - this_buf->start);
        } else {
            size = (this_buf->len - this_buf->start + this_buf->end);
        }
    }

    return size;
}

// initialize the free record buffer
void free_records_init() {
    payload_buffer_init(&free_records);
    for (uint8_t i = 0; i < ALL_FREE_RECORDS; i++) {
        payload_buffer_push(&free_records, &records[i]);
    }
}

struct payload_record *payload_record_alloc() {
    return payload_buffer_pop(&free_records);
}

bool payload_record_free(struct payload_record *in) {
    return payload_buffer_push(&free_records, in);
}

uint8_t n_free_records() {
    return payload_buffer_capacity(&free_records);
}