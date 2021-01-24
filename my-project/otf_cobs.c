#include "otf_cobs.h"
#include <stdbool.h>
#include <sys/types.h>

enum cobs_decode_result cobs_decode_buf_push_char(struct cobs_decode_buf *decode_buf, char c){

    if(decode_buf->frame_is_terminated){
        return TERMINATED;
    }

    if (c == COBS_DELIMETER) {
		if ( decode_buf->next_zero_pos == decode_buf->pos){
			decode_buf->frame_is_terminated = true;
			return TERMINATED;
		} else {
			return ENCODING_ERROR;
		}
		
	}

	if ( !(decode_buf->pos<COBS_RX_BUF_SIZE) ){
		return OVERFLOW;
	}

	if ( decode_buf->next_zero_pos == decode_buf->pos ){ //at a zero position
		
		if(decode_buf->next_zero_is_overhead){ //this zero position is an overhead byte
			decode_buf->next_zero_pos += c-1; //update where the next zero position is, accounting for the fact that nothing will be written to the buffer
		}else{//this zero position is not overhead
			decode_buf->next_zero_pos += c;
			decode_buf->buf[decode_buf->pos] = COBS_DELIMETER; //fill in the zero
			decode_buf->pos++; //move to the next character in the buffer
		}

		//determine if the following zero position is overhead
		if(c == 0xFF){
			decode_buf->next_zero_is_overhead = true;
		} else {
			decode_buf->next_zero_is_overhead = false;
		}
		
	} else {	//not at a zero position
		decode_buf->buf[decode_buf->pos] = c;
		decode_buf->pos++;
	}

    return OK;
}

ssize_t cobs_encode_buf_push(struct cobs_encode_buf *encode_buf, char *buf, size_t n){

    if( encode_buf->frame_is_terminated ) return 0;

    uint16_t i = 0;

	while(encode_buf->pos < COBS_TX_BUF_SIZE-ENCODING_OVH_END && i<n){

		if( (encode_buf->pos - encode_buf->last_zero_pos) != 0xFF ){
			//overhead byte not needed
			if(buf[i] != COBS_DELIMETER){
				//next byte in input buffer is not zero
				encode_buf->buf[encode_buf->pos] = encode_buf[i]; //write it
				//move to next character in the input buffer
			}else{
				//next byte in intput buffer is zero
				encode_buf->buf[encode_buf->last_zero_pos] = encode_buf->pos - encode_buf->last_zero_pos; //fill in the last zero position with the distance to this zero
				encode_buf->last_zero_pos = encode_buf->pos; //record the position of this next zero

			}
			i++; //move ahead to the next bute in the input buffer
			encode_buf->pos++; //move ahead to the next byte in the tx buffer
		} else {
			//254 bytes of nonzero characters precede this point in the buffer; a non-data overhead byte is needed
			encode_buf->buf[encode_buf->last_zero_pos] = 0xFF; //fill in the last zero position with the distance to this overhead byte
			encode_buf->last_zero_pos = encode_buf->pos; //record the position of this zero position
			encode_buf->pos++; //move ahead in the tx buffer but not the input buffer
		}

	}
	
	return i;
}

void cobs_encode_buf_terminate(struct cobs_encode_buf *encode_buf){
    encode_buf->buf[encode_buf->pos] = COBS_DELIMETER;
    encode_buf->frame_is_terminated = true;
}

static void cobs_buffer_reset(void *cobs_buffer) {
	enum cobs_buf_type buf_type = *( (uint8_t *) cobs_buffer);

	switch(buf_type){
		case ENCODE_BUF:
			{
				struct cobs_encode_buf *buf = (struct cobs_encode_buf *) cobs_buffer;
				buf->last_zero_pos = 0; //let writer write over the first byte to indicate position of next zero
				buf->pos = ENCODING_OVH_START; //start writing at beginning of data section
                buf->frame_is_terminated = false;
				break;
			}
		case DECODE_BUF:
			{
				struct cobs_decode_buf *buf = (struct cobs_decode_buf *) cobs_buffer;
				buf->next_zero_pos = 0;
				buf->pos = 0;
				buf->next_zero_is_overhead = true;
				buf->frame_is_terminated = false;
				break;
			}
	}
}