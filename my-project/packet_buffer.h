#include "sx127x.h"
#include <sys/types.h>
#include <stdbool.h>

#define PACKET_BUFFER_LENGTH 3 //NOTE: This is arbitrary

struct packet_record{
    uint8_t packet[MAX_PAYLOAD_LENGTH];
    uint8_t length;
};

struct packet_buf {
    uint8_t start;
    uint8_t end;
    struct packet_record *records;
    bool full;
};

void packet_buf_init(struct packet_buf *this_buf, struct packet_record record_arr[PACKET_BUFFER_LENGTH]);

//returns null if overflow
struct packet_record *packet_buf_push(struct packet_buf *this_buf);

//returns null if empty
struct packet_record *packet_buf_pop(struct packet_buf *this_buf);

struct packet_record *packet_buf_peek(struct packet_buf *this_buf);

uint8_t packet_buf_get_cap(struct packet_buf *this_buf);

extern struct packet_buf packet_buffer;
