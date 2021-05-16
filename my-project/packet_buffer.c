#include "sx127x.h"
#include <sys/types.h>
#include <stdbool.h>
#include "packet_buffer.h"

void packet_buf_init(struct packet_buf *this_buf, struct packet_record record_arr[PACKET_BUFFER_LENGTH]){
    this_buf->start = 0;
    this_buf->end = 0;
    this_buf->full = false;
    this_buf->records = record_arr;
    return;
}

//returns null if overflow
struct packet_record *packet_buf_push(struct packet_buf *this_buf){
    if(this_buf->full) {
        return (struct packet_record *) NULL;
    } else {
        struct packet_record *retval = &(this_buf->records[this_buf->end]);
        if(this_buf->end==(PACKET_BUFFER_LENGTH-1)){
            this_buf->end = 0;
        }else{
            this_buf->end++;
        }
        if (this_buf->end==this_buf->start) this_buf->full = true;
        return retval;
    }
}

//returns null if empty
struct packet_record *packet_buf_pop(struct packet_buf *this_buf){
    if( !(this_buf->full) && this_buf->start == this_buf->end){
        return (struct packet_record *) NULL;
    }
    this_buf->full = false;

    struct packet_record *retval = &(this_buf->records[this_buf->start]);

    if(this_buf->start == (PACKET_BUFFER_LENGTH - 1)){
        this_buf->start = 0;
    }else{
        this_buf->start++;
    }

    return retval;
}

/**
 * @brief peek at the next element out of the buffer
 * 
 * The purpose of this function is to allow the caller to get information out of the buffer
 * without having the space occupied with new data.
 * 
 */
struct packet_record *packet_buf_peek(struct packet_buf *this_buf){
    if( !(this_buf->full) && this_buf->start == this_buf->end){
        return (struct packet_record *) NULL;
    }

    struct packet_record *retval = &(this_buf->records[this_buf->start]);
    return retval;
}

/* taken from https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/ */
uint8_t packet_buf_get_cap(struct packet_buf *this_buf){
    uint8_t size = PACKET_BUFFER_LENGTH;

	if(!this_buf->full)
	{
		if(this_buf->end >= this_buf->start)
		{
			size = (this_buf->end - this_buf->start);
		}
		else
		{
			size = (PACKET_BUFFER_LENGTH - this_buf->start + this_buf->end);
		}
	}

	return size;
}