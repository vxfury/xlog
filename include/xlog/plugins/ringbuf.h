#ifndef __RINGBUF_H
#define __RINGBUF_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

/** memory allocation for ring-buffer */
#define RBUF_MALLOC(nbytes)			((nbytes) == 0 ? NULL : calloc(1, nbytes))
#define RBUF_REALLOC(ptr, nbytes)	realloc(ptr, nbytes)
#define RBUF_FREE(ptr)				free(ptr)
#define RBUF_STRDUP(str)			strdup(str)
#define RBUF_STRNDUP(str, n)		strndup(str, n)

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __ringbuf {
	pthread_mutex_t mutex;
	pthread_cond_t  cond_data_out;
	pthread_cond_t  cond_data_in;
	
	unsigned int capacity; /* capacity = size - 1, one byte for detecting the full condition. */
	unsigned int rd_offset, wr_offset;
	char *data;
} ringbuf_t;

/* @brief  create ring-buffer
 * @param  capacity, capacity of ring-buffer
 * @return pointer to ring-buffer; NULL if failed to allocate memory
 * @note   1. Align will be applied automatically
 *         2. One byte additional for detecting the full condition
 **/
ringbuf_t *ringbuf_create( unsigned int capacity );

/* @brief  destory ring-buffer
 * @param  rb, pointer to ring-buffer
 * @return error code(always zero for this interface)
 **/
int ringbuf_destory( ringbuf_t *rb );

/* @brief  copy data to ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, data to copy into
 *         block_size, minimal size of block to copy each time
 * @return error code(always zero for this interface)
 * @note   1. data given may be separated into several fragments
 *         2. but NO LIMIT on size of data
 **/
int ringbuf_copy_into_separable( ringbuf_t *rb, const void *vptr, unsigned int size, unsigned int block_size );

/* @brief  copy data to ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, data to copy into ring-buffer
 * @return error code(EINVAL if size of data larger than half of the capacity)
 **/
int ringbuf_copy_into( ringbuf_t *rb, const void *vptr, unsigned int size );

/* @brief  copy data from ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, buffer to save copied data
 *         no_wait, true to return immediately when there is no data in ring-buffer
 * @return length of copied data
 **/
int ringbuf_copy_from( ringbuf_t *rb, void *vptr, unsigned int size, bool no_wait );

/* @brief  Locate the first occurrence of character c in ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         c, character to locate
 *         offset, satrt offset to find character
 * @return offset of the character from the ring-buffer's tail pointer
 * @note   non-thread-safe
 **/
unsigned int ringbuf_findchr( const ringbuf_t *rb, int c, unsigned int offset );

#ifdef __cplusplus
}
#endif

#endif
