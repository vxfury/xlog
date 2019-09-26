#include "plugins/ringbuf.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

#define RBUF_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define RBUF_ALIGN_UP(size, align) ((align+size-1) & (~(align-1)))
#define RBUF_TRACE(...)

/* @brief  create ring-buffer
 * @param  capacity, capacity of ring-buffer
 * @return pointer to ring-buffer; NULL if failed to allocate memory
 * @note   1. Align will be applied automatically
 *         2. One byte additional for detecting the full condition
 **/
ringbuf_t *ringbuf_create( unsigned int capacity )
{
	/* One byte used for detecting the full condition. */
	/* Align is applied 'cause the capacity usually very large for better performance */
	ringbuf_t *rb = (ringbuf_t *)RBUF_MALLOC( sizeof( ringbuf_t ) );
	if( rb ) {
		size_t size = RBUF_ALIGN_UP( capacity + 1, 1024 );
		rb->data = (char *)RBUF_MALLOC( size );
		if( rb->data == NULL ) {
			RBUF_TRACE( "Failed to allocate memory." );
			RBUF_FREE( rb );
			return NULL;
		}
		rb->capacity = size - 1;
		rb->rd_offset = 0;
		rb->wr_offset = 0;
		
		pthread_mutex_init( &rb->mutex, NULL );
		pthread_cond_init( &rb->cond_data_out, NULL );
		pthread_cond_init( &rb->cond_data_in, NULL );
	}
	
	return rb;
}

/* @brief  destory ring-buffer
 * @param  rb, pointer to ring-buffer
 * @return error code(always zero for this interface)
 **/
int ringbuf_destory( ringbuf_t *rb )
{
	if( rb ) {
		pthread_mutex_destroy( &rb->mutex );
		pthread_cond_destroy( &rb->cond_data_in );
		pthread_cond_destroy( &rb->cond_data_out );
		RBUF_FREE( rb->data );
		RBUF_FREE( rb );
	}
	
	return 0;
}

/* offset to next n bytes */
static unsigned int __offset_next_n( const ringbuf_t* rb, int offset, int n )
{
	assert( ( offset >= 0 ) && ( offset <= rb->capacity ) );
	return ( offset + n ) % ( rb->capacity + 1 );
}

/* get free size */
static unsigned int __size_free( const ringbuf_t *rb )
{
	assert( rb );
	unsigned int bytes_free = 0;
	if( rb->wr_offset < rb->rd_offset ) {
		bytes_free = rb->rd_offset - rb->wr_offset - 1;
	} else {
		bytes_free = rb->capacity - (rb->wr_offset - rb->rd_offset);
	}
	
	return bytes_free;
}

/* get used size  */
static unsigned int __size_used( const ringbuf_t *rb )
{
	assert( rb );
	unsigned int bytes_used = 0;
	if( rb->wr_offset < rb->rd_offset ) {
		bytes_used = rb->capacity - (rb->rd_offset - rb->wr_offset - 1);
	} else {
		bytes_used = rb->wr_offset - rb->rd_offset;
	}
	
	return bytes_used;
}

/* @brief  copy data to ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, data to copy into
 *         block_size, minimal size of block to copy each time
 * @return error code(always zero for this interface)
 * @note   1. data given may be separated into several fragments
 *         2. but NO LIMIT on size of data
 **/
int ringbuf_copy_into_separable( ringbuf_t *rb, const void *vptr, unsigned int size, unsigned int block_size )
{
	assert( rb );
	unsigned int left_size = size;
	while( left_size > 0 ) {
		pthread_mutex_lock( &rb->mutex );
		while( __size_free(rb) < block_size ) {
			pthread_cond_wait( &rb->cond_data_out, &rb->mutex );
		}
		unsigned int copy_size = RBUF_MIN( left_size, __size_free(rb) );
		unsigned int next_wr = __offset_next_n( rb, rb->wr_offset, copy_size );
		unsigned int non_overflow_size = RBUF_MIN( copy_size, rb->capacity + 1 - rb->wr_offset );
		memcpy( rb->data + rb->wr_offset, vptr, non_overflow_size );
		if( non_overflow_size < copy_size ) {
			memcpy( rb->data, (char *)vptr + non_overflow_size, copy_size - non_overflow_size );
		}
		left_size -= copy_size;
		RBUF_TRACE( "INTO: free/capacity = %u/%u, non-overflow-size/read-length = %u/%u, next_wr = %u", __size_free( rb ), rb->capacity, non_overflow_size, copy_size, next_wr );
		rb->wr_offset = next_wr;
		pthread_cond_broadcast( &rb->cond_data_in );
		pthread_mutex_unlock( &rb->mutex );
	}
	
	return 0;
}

/* @brief  copy data to ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, data to copy into ring-buffer
 * @return error code(EINVAL if size of data larger than half of the capacity)
 **/
int ringbuf_copy_into( ringbuf_t *rb, const void *vptr, unsigned int size )
{
	assert( rb );
	if( size > ( rb->capacity >> 1 ) ) {
		RBUF_TRACE( "Buffer required cann't be satisfied by pre-created ring-buffer." );
		return EINVAL;
	}
	ringbuf_copy_into_separable( rb, vptr, size, size );
	
	return 0;
}

/* @brief  copy data from ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         vptr/size, buffer to save copied data
 *         no_wait, true to return immediately when there is no data in ring-buffer
 * @return length of copied data
 **/
int ringbuf_copy_from( ringbuf_t *rb, void *vptr, unsigned int size, bool no_wait )
{
	pthread_mutex_lock( &rb->mutex );
	if( no_wait ) {
		if( __size_used(rb) == 0 ) {
			pthread_mutex_unlock( &rb->mutex );
			return 0;
		}
	} else {
		while( __size_used(rb) == 0 ) {
			pthread_cond_wait( &rb->cond_data_in, &rb->mutex );
		}
	}
	
	unsigned int bytes_used = __size_used( rb );
	unsigned int length = RBUF_MIN( bytes_used, size );
	unsigned int next_rd = __offset_next_n( rb, rb->rd_offset, length );
	unsigned int non_overflow_size = RBUF_MIN( length, rb->capacity + 1 - rb->rd_offset );
	memcpy( vptr, rb->data + rb->rd_offset, non_overflow_size );
	if( non_overflow_size < length ) {
		memcpy( (void *)( (char *)vptr + non_overflow_size ), rb->data, length - non_overflow_size );
	}
	RBUF_TRACE( "FROM: used/capacity = %u/%u, non-overflow-size/read-length = %u/%u, next_rd = %u", bytes_used, rb->capacity, non_overflow_size, length, next_rd );
	rb->rd_offset = next_rd;
	pthread_cond_broadcast( &rb->cond_data_out );
	pthread_mutex_unlock( &rb->mutex );
	
	return length;
}

/* @brief  Locate the first occurrence of character c in ring-buffer
 * @param  rb, pointer to ring-buffer(Non-NULL required)
 *         c, character to locate
 *         offset, satrt offset to find character
 * @return offset of the character from the ring-buffer's tail pointer
 * @note   non-thread-safe
 **/
unsigned int ringbuf_findchr( const ringbuf_t *rb, int c, unsigned int offset )
{
	assert( rb );
	unsigned int bytes_used = __size_used( rb );
	if( offset >= bytes_used ) {
		return bytes_used;
	}
	
	const char *start = rb->data + __offset_next_n( rb , rb->rd_offset, offset );
	unsigned int n = RBUF_MIN( bytes_used - offset, rb->capacity + 1 - rb->rd_offset );
	const char *found = (const char *)memchr( start, c, n );
	if( found ) {
		return offset + ( found - start );
	} else {
		return ringbuf_findchr( rb, c, offset + n );
	}
}
