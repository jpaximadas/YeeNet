
#pragma once

enum packet_type { ACK, NACK, DATA_ACKED, DATA_UNACKED };

struct packet_ack {
    uint8_t type;
    uint8_t src;
    uint8_t dest;
};
struct packet_nack {
    uint8_t type;
    uint8_t src;
    uint8_t dest;
};

#define PACKET_DATA_OVERHEAD 4
// packet_data_overhead + len = number of bytes in entire struct
struct packet_data {
    uint8_t type;
    uint8_t src;
    uint8_t dest;
    uint8_t len;
    uint8_t data[MAX_PAYLOAD_LENGTH - PACKET_DATA_OVERHEAD];
};
