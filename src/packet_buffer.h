/**
 * @file packet_buffer.h
 *
 * Provides a FIFO for incoming packets.
 */

#include "sx127x.h"
#include <stdbool.h>
#include <sys/types.h>

#define PACKET_BUFFER_LENGTH 3  // NOTE: This is arbitrary
extern uint32_t n_overflow;     // Keeps track of overflow events

/**
 * @struct packet_record
 *
 * Represnets an incoming packet
 */
struct packet_record {
    /**
     * Can store an entire LoRa payload
     */
    uint8_t packet[MAX_PAYLOAD_LENGTH];
    /**
     * Length of the data inside the packet array
     */
    uint8_t length;
    /**
     * RSSI of the packet
     */
    int32_t rssi;
    /**
     * SNR during packet reception
     */
    float snr;
};

/**
 * Struct for managing an array of packet_records in a ring buffer
 */
struct packet_buf {
    /**
     * Starting position of the ring buffer
     */
    uint8_t start;
    /**
     * End of the ring buffer
     */
    uint8_t end;
    /**
     * pointer to an array of packet records to be managed
     */
    struct packet_record *records;
    /**
     * Indicates if the buffer is full
     * This disambiguates the start==end condition. (can happen if the buffer is full or empty)
     */
    bool full;
};

/**
 * @brief initialize a packet buffer
 * This should be run before any packets are popped or pushed.
 *
 * @param this_buf pointer to a packet buffer struct
 * @param record_arr array of records to be managed
 */
void packet_buf_init(struct packet_buf *this_buf, struct packet_record record_arr[PACKET_BUFFER_LENGTH]);

/**
 * @brief push a packet record into the FIFO
 * This function will return a pointer to a packet record that is now allocated
 * Note that the this buffering system does not have a way to know if the caller is done filling the record
 * with information. An interrupt with high priority could potentially read it out before it's initialized.
 *
 * @param this_buf pointer to packet buffer
 * @return pointer to a free packet record
 */
struct packet_record *packet_buf_push(struct packet_buf *this_buf);

/**
 * @brief pop a packet out of the FIFO
 * This function will free and return a pointer to the first out packet record
 * Note that when this occurs, that packet record is immediately available to be reallocated on a push.
 * Use peek if the caller needs time to get the data out of the packet record.
 *
 * @param this_buf pointer to packet buffer
 * @return pointer to first out packet record
 */
struct packet_record *packet_buf_pop(struct packet_buf *this_buf);

/**
 * @brief peek at the first out packet record
 * This function will return the first out packet record from the buffer but will not free it.
 * This can give the caller time to read out the data before popping it.
 *
 * @param this_buf pointer to packet buffer
 * @return pointer to first out packet record
 */
struct packet_record *packet_buf_peek(struct packet_buf *this_buf);

/**
 * @brief measure how full the buffer is
 *
 * @param this_buf pointer to packet_buffer
 * @return number of packets in the buffer
 */
uint8_t packet_buf_get_cap(struct packet_buf *this_buf);

extern struct packet_buf packet_buffer;
