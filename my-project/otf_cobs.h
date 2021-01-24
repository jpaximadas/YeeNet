#include <stdbool.h>
#include <sys/types.h>

#define COBS_DELIMETER 0x00
#define COBS_TX_BUF_SIZE 300
#define COBS_RX_BUF_SIZE 300
#define ENCODING_OVH_START 1
#define ENCODING_OVH_END 1

enum cobs_buf_type{
    ENCODE_BUF,
    DECODE_BUF
};

void cobs_buf_reset(void *cobs_buf);

/* DECODING */

struct cobs_decode_buf{
	enum cobs_buf_type buftype;
	uint8_t buf[COBS_RX_BUF_SIZE];
	uint16_t pos;
	uint16_t next_zero_pos;
	bool next_zero_is_overhead;
	bool frame_is_terminated;
};

enum cobs_decode_result{
    OK,
    TERMINATED,
    OVERFLOW,
    ENCODING_ERROR
};

enum cobs_decode_result cobs_decode_buf_push_char(struct cobs_decode_buf *decode_buf, char c);

/* ENCODING */

struct cobs_encode_buf{
	enum cobs_buf_type buftype;
	uint8_t buf[COBS_TX_BUF_SIZE];
	uint16_t pos;
	uint16_t last_zero_pos;
    bool frame_is_terminated;
};

ssize_t cobs_encode_buf_push(struct cobs_encode_buf *encode_buf, char *buf, size_t n);

void cobs_encode_buf_terminate(struct cobs_encode_buf *encode_buf);
