#ifndef __INTERNAL_RINGBUF_H
#define __INTERNAL_RINGBUF_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __ringbuf {
	pthread_mutex_t mutex;
	pthread_cond_t  cond_data_out;
	pthread_cond_t  cond_data_in;
	
	unsigned int capacity; /* capacity = size - 1, one byte for detecting the full condition. */
	unsigned int rd_offset, wr_offset;
	unsigned char data[0];
} ringbuf_t;

ringbuf_t *ringbuf_create( unsigned int capacity );
int ringbuf_destory( ringbuf_t *rb );
int ringbuf_copy_into_nonspec( ringbuf_t *rb, const void *vptr, unsigned int size );
int ringbuf_copy_into( ringbuf_t *rb, const void *vptr, unsigned int size );
int ringbuf_copy_from( ringbuf_t *rb, void *vptr, unsigned int size, bool no_wait );

#ifdef __cplusplus
}
#endif

#endif
