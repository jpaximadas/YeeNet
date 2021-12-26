/**
 * @file otf_cobs.h
 *
 * This file provides function to perform on-the-fly encoding or decoding a COBS frame.
 * A COBS decode buffer will decode a COBS frame as each byte is pushed.
 * A COBS encode buffer will encode bytes into a COBS frame as each bytes is pushed.
 * The COBS decode buffer push function is meant to run during a UART RX interrupt and removes the need for an
 * extra array for COBS decoding. The COBS encode buffer push function is meant to be run by the host link and
 * host link jump table functions and removes the need for an extra array for COBS encoding.
 */

#include <stdbool.h>
#include <sys/types.h>
#pragma once

/**
 * Buffer sizing is set to accommodate the longest packet that the interface will have to handle in either
 * direction. An incoming packet size is set to be larger 255 so the interface an accommodate a whole LoRa
 * payload and overhead for COBS, selecting a command, and other data. The choice of 300 exactly is arbitrary.
 * The longest response the host link can generate is through the echo command.
 * A 1 byte command and 299 byte echo argument will generate at most a 303 byte response.
 */
#define COBS_DELIMETER 0x00   // indicates end of frame
#define COBS_TX_BUF_SIZE 303  // size of TX buffer
#define COBS_RX_BUF_SIZE 300  // size of TX buffer
#define ENCODING_OVH_START 1
#define ENCODING_OVH_END 1

/**
 * Enum to define what kind of buffer a cobs buffer is to support polymorphism
 */
enum cobs_buf_type { ENCODE_BUF, DECODE_BUF };

/**
 * @brief reset a COBS buffer
 *
 * @param cobs_buf pointer to a cobs buffer struct
 */
void cobs_buf_reset(void *cobs_buf);

/* DECODING */

/**
 * @struct cobs_decode_buf
 *
 * @brief represents a COBS decode buffer.
 * This data type contains a buffer of size COBS_RX_BUF_SIZE and some variables to make COBS decoding work.
 */
struct cobs_decode_buf {
    /**
     * Indicates what kind of buffer this struct is.
     * Should always be DECODE_BUF
     */
    enum cobs_buf_type buftype;
    /**
     * Array of bytes for the COBS decoded packet
     */
    uint8_t buf[COBS_RX_BUF_SIZE];
    /**
     * tracks the position of the next free byte
     */
    uint16_t pos;
    /**
     * Keeps track of where the next zero or overhead byte is
     */
    uint16_t next_zero_pos;
    /**
     * Indicates whether next_zero_pos points to a zero byte or overhead
     */
    bool next_zero_is_overhead;
    /**
     * Indicates if the frame in the buffer has finished being decoded (delimiter was received)
     */
    bool frame_is_terminated;
};

/**
 * Enum for cobs_decode return
 */
enum cobs_decode_result {
    /**
     * returned if the decoding was successful
     */
    OK,
    /**
     * returned if the buffer is already terminated
     */
    TERMINATED,
    /**
     * returned if the buffer overflows and more bytes can't be written
     */
    OVERFLOW,
    /**
     * returned if a cobs delimiter is pushed in an unexpected location
     */
    ENCODING_ERROR
};

/**
 * @brief pushes a byte to a cobs_decode buffer
 *
 * @param decode_buf pointer to a decode buffer struct
 * @param c the character to be written
 *
 * @return the outcome of the attempt to decode the byte
 */
enum cobs_decode_result cobs_decode_buf_push_char(struct cobs_decode_buf *decode_buf, uint8_t c);

/* ENCODING */

/**
 * @struct cobs_encode_buf
 *
 * @brief represents a COBS encode buffer
 *
 * This data type contains a buffer of size COBS_TX_BUF_SIZE and some additional variables to make encoding
 * possible.
 */
struct cobs_encode_buf {
    /**
     * Indicates the type of the buffer. Must always be ENCODE_BUF
     */
    enum cobs_buf_type buftype;
    /**
     * Buffer for the encoded packet. Read from here when the whole frame is done encoding.
     */
    uint8_t buf[COBS_TX_BUF_SIZE];
    /**
     * tracks position of the next free byte
     */
    uint16_t pos;
    /**
     * points to the last position of a zero byte
     */
    uint16_t last_zero_pos;
    /**
     * indicates whether a frame is terminated
     */
    bool frame_is_terminated;
};

/**
 * @brief push bytes to the encode buffer
 *
 * @param encode_buf pointer to an encode buffer
 * @param buf pointer to bytes to be written
 * @param n number of bytes to be written
 *
 * @return the number of bytes that were successfully written
 */
ssize_t cobs_encode_buf_push(struct cobs_encode_buf *encode_buf, uint8_t *buf, size_t n);

/**
 * @brief push a byte to the encode buffer
 *
 * @param encode_buf pointer to an encode buffer
 * @param c byte to write
 *
 * @return number of bytes that were successfully written
 */
ssize_t cobs_encode_buf_push_char(struct cobs_encode_buf *encode_buf, uint8_t c);

/**
 * @brief terminate an encode buffer
 * Calling this function will tell the encoding buffer there aren't any more bytes to write.
 * This will place the COBS delimiter at the end of the buffer and block any subsequent pushes
 *
 * @param encode_buf pointer to encode buffer
 */
void cobs_encode_buf_terminate(struct cobs_encode_buf *encode_buf);
