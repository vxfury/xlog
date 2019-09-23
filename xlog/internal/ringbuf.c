#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "xlog.h"
#include "ringbuf.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

ringbuf_t *ringbuf_create( unsigned int capacity )
{
	/* One byte used for detecting the full condition. */
	/* Align is applied 'cause the capacity usually very large for better performance */
	size_t size = XLOG_ALIGN_UP( sizeof( ringbuf_t ) + capacity + 1, 64 );
	ringbuf_t *rb = (ringbuf_t *)XLOG_MALLOC( size );
	if( rb ) {
		rb->capacity = capacity;
		rb->rd_offset = 0;
		rb->wr_offset = 0;
		pthread_mutex_init( &rb->mutex, NULL );
		pthread_cond_init( &rb->cond_data_out, NULL );
	}
	
	return rb;
}

int ringbuf_destory( ringbuf_t *rb )
{
	if( rb ) {
		pthread_mutex_destroy( &rb->mutex );
		pthread_cond_destroy( &rb->cond_data_in );
		pthread_cond_destroy( &rb->cond_data_out );
		XLOG_FREE( rb );
	}
	
	return 0;
}

static unsigned int __offset_next_n( ringbuf_t* rb, int offset, int n )
{
	assert( ( offset >= 0 ) && ( offset <= rb->capacity ) );
	return ( offset + n ) % ( rb->capacity + 1 );
}

static unsigned int __size_free( ringbuf_t *rb )
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

static unsigned int __size_used( ringbuf_t *rb )
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

int ringbuf_copy_into_nonspec( ringbuf_t *rb, const void *vptr, unsigned int size )
{
	assert( rb );
	unsigned int left_size = size;
	while( left_size > 0 ) {
		pthread_mutex_lock( &rb->mutex );
		while( __size_free(rb) == 0 ) {
			pthread_cond_wait( &rb->cond_data_out, &rb->mutex );
		}
		unsigned int copy_size = XLOG_MIN( left_size, __size_free(rb) );
		unsigned int next_wr = __offset_next_n( rb, rb->wr_offset, copy_size );
		unsigned int non_overflow_size = XLOG_MIN( copy_size, rb->capacity + 1 - rb->wr_offset );
		memcpy( rb->data + rb->wr_offset, vptr, non_overflow_size );
		if( non_overflow_size < copy_size ) {
			memcpy( rb->data, (char *)vptr + non_overflow_size, copy_size - non_overflow_size );
		}
		left_size -= copy_size;
		__XLOG_TRACE( "INTO: free/capacity = %u/%u, non-overflow-size/read-length = %u/%u, next_wr = %u", __size_free( rb ), rb->capacity, non_overflow_size, copy_size, next_wr );
		rb->wr_offset = next_wr;
		pthread_cond_broadcast( &rb->cond_data_in );
		pthread_mutex_unlock( &rb->mutex );
	}
	
	return 0;
}

int ringbuf_copy_into( ringbuf_t *rb, const void *vptr, unsigned int size )
{
	assert( rb );
	if( size > ( rb->capacity >> 1 ) ) {
		__XLOG_TRACE( "Buffer required cann't be satisfied by pre-created ring-buffer." );
		return EINVAL;
	}
	pthread_mutex_lock( &rb->mutex );
	while( __size_free(rb) < size ) {
		pthread_cond_wait( &rb->cond_data_out, &rb->mutex );
	}
	unsigned int copy_size = XLOG_MIN( size, __size_free(rb) );
	unsigned int next_wr = __offset_next_n( rb, rb->wr_offset, copy_size );
	unsigned int non_overflow_size = XLOG_MIN( copy_size, rb->capacity + 1 - rb->wr_offset );
	memcpy( rb->data + rb->wr_offset, vptr, non_overflow_size );
	if( non_overflow_size < copy_size ) {
		memcpy( rb->data, (char *)vptr + non_overflow_size, copy_size - non_overflow_size );
	}
	__XLOG_TRACE( "INTO: free/capacity = %u/%u, non-overflow-size/read-length = %u/%u, next_wr = %u", __size_free( rb ), rb->capacity, non_overflow_size, copy_size, next_wr );
	rb->wr_offset = next_wr;
	pthread_cond_broadcast( &rb->cond_data_in );
	pthread_mutex_unlock( &rb->mutex );
	
	return 0;
}

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
	unsigned int length = XLOG_MIN( bytes_used, size );
	unsigned int next_rd = __offset_next_n( rb, rb->rd_offset, length );
	unsigned int non_overflow_size = XLOG_MIN( length, rb->capacity + 1 - rb->rd_offset );
	memcpy( vptr, rb->data + rb->rd_offset, non_overflow_size );
	if( non_overflow_size < length ) {
		memcpy( (void *)( (char *)vptr + non_overflow_size ), rb->data, length - non_overflow_size );
	}
	__XLOG_TRACE( "FROM: used/capacity = %u/%u, non-overflow-size/read-length = %u/%u, next_rd = %u", bytes_used, rb->capacity, non_overflow_size, length, next_rd );
	rb->rd_offset = next_rd;
	pthread_cond_broadcast( &rb->cond_data_out );
	pthread_mutex_unlock( &rb->mutex );
	
	return length;
}

#if 0
#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif

typedef struct ringbuf_t {
	pthread_mutex_t mutex;
	pthread_cond_t  cond_data_out;
	pthread_cond_t  cond_data_in;
	
	uint8_t *buf;
	uint8_t *head, *tail;
	size_t capacity;
} ringbuf_t;

ringbuf_t* ringbuf_new( size_t capacity );
void ringbuf_free( ringbuf_t* *rb );
void ringbuf_reset( ringbuf_t* rb );
size_t ringbuf_bytes_free( const struct ringbuf_t* *rb );
size_t ringbuf_bytes_used( const struct ringbuf_t* *rb );
int ringbuf_is_full( const struct ringbuf_t* *rb );
int ringbuf_is_empty( const struct ringbuf_t* *rb );
const void *ringbuf_tail( const struct ringbuf_t* *rb );
const void *ringbuf_head( const struct ringbuf_t* *rb );
size_t ringbuf_findchr( const struct ringbuf_t* *rb, int c, size_t offset );
size_t ringbuf_memset( ringbuf_t* dst, int c, size_t len );
void *ringbuf_memcpy_into( ringbuf_t* dst, const void *src, size_t count );
void *ringbuf_memcpy_from( void *dst, ringbuf_t* src, size_t count );

ringbuf_t* ringbuf_new( size_t capacity )
{
	ringbuf_t* rb = malloc( sizeof( struct ringbuf_t* ) );
	if( rb ) {
		/* One byte is used for detecting the full condition. */
		rb->capacity = capacity;
		rb->buf = malloc( capacity + 1 );
		if( rb->buf ) {
			ringbuf_reset( rb );
		} else {
			free( rb );
			return 0;
		}
	}
	return rb;
}

void ringbuf_free( ringbuf_t* *rb )
{
	assert( rb && *rb );
	free( ( *rb )->buf );
	free( *rb );
	*rb = 0;
}

void ringbuf_reset( ringbuf_t* rb )
{
	rb->head = rb->tail = rb->buf;
}

/*
 * Return a pointer to one-past-the-end of the ring buffer's
 * contiguous buffer. You shouldn't normally need to use this function
 * unless you're writing a new ringbuf_* function.
 */
static const uint8_t *ringbuf_end( const struct ringbuf_t* *rb )
{
	return rb->buf + rb->capacity + 1;
}

size_t ringbuf_bytes_free( const struct ringbuf_t* *rb )
{
	if( rb->head >= rb->tail ) {
		return rb->capacity - (size_t)( rb->head - rb->tail );
	} else {
		return (size_t)(rb->tail - rb->head) - 1;
	}
}

size_t ringbuf_bytes_used( const struct ringbuf_t* *rb )
{
	return rb->capacity - ringbuf_bytes_free( rb );
}

int ringbuf_is_full( const struct ringbuf_t* *rb )
{
	return ringbuf_bytes_free( rb ) == 0;
}

int ringbuf_is_empty( const struct ringbuf_t* *rb )
{
	return ringbuf_bytes_free( rb ) == rb->capacity;
}

const void *ringbuf_tail( const struct ringbuf_t* *rb )
{
	return rb->tail;
}

const void *ringbuf_head( const struct ringbuf_t* *rb )
{
	return rb->head;
}

/*
 * Given a ring buffer rb and a pointer to a location within its
 * contiguous buffer, return the a pointer to the next logical
 * location in the ring buffer.
 */
static uint8_t *ringbuf_nextp( ringbuf_t* rb, const uint8_t *p )
{
	/*
	 * The assert guarantees the expression (++p - rb->buf) is
	 * non-negative; therefore, the modulus operation is safe and
	 * portable.
	 */
	assert( ( p >= rb->buf ) && ( p < ringbuf_end( rb ) ) );
	return rb->buf + ( ( ++p - rb->buf ) % ( rb->capacity + 1 ) );
}

size_t ringbuf_findchr( const struct ringbuf_t* *rb, int c, size_t offset )
{
	const uint8_t *bufend = ringbuf_end( rb );
	size_t bytes_used = ringbuf_bytes_used( rb );
	if( offset >= bytes_used ) {
		return bytes_used;
	}
	
	const uint8_t *start = rb->buf +
	    ( ( (size_t)( rb->tail - rb->buf ) + offset ) % ( rb->capacity + 1 ) );
	assert( bufend > start );
	size_t n = MIN( bufend - start, bytes_used - offset );
	const uint8_t *found = memchr( start, c, n );
	if( found ) {
		return offset + ( found - start );
	} else {
		return ringbuf_findchr( rb, c, offset + n );
	}
}

size_t ringbuf_memset( ringbuf_t* dst, int c, size_t len )
{
	const uint8_t *bufend = ringbuf_end( dst );
	size_t nwritten = 0;
	size_t count = MIN( len, dst->capacity + 1 );
	int overflow = count > ringbuf_bytes_free( dst );
	
	while( nwritten != count ) {
	
		/* don't copy beyond the end of the buffer */
		assert( bufend > dst->head );
		size_t n = MIN( bufend - dst->head, count - nwritten );
		memset( dst->head, c, n );
		dst->head += n;
		nwritten += n;
		
		/* wrap? */
		if( dst->head == bufend ) {
			dst->head = dst->buf;
		}
	}
	
	if( overflow ) {
		dst->tail = ringbuf_nextp( dst, dst->head );
		assert( ringbuf_is_full( dst ) );
	}
	
	return nwritten;
}

void *ringbuf_memcpy_into( ringbuf_t* dst, const void *src, size_t count )
{
	const uint8_t *u8src = src;
	const uint8_t *bufend = ringbuf_end( dst );
	int overflow = count > ringbuf_bytes_free( dst );
	size_t nread = 0;
	
	while( nread != count ) {
		/* don't copy beyond the end of the buffer */
		assert( bufend > dst->head );
		size_t n = MIN( bufend - dst->head, count - nread );
		memcpy( dst->head, u8src + nread, n );
		dst->head += n;
		nread += n;
		
		/* wrap? */
		if( dst->head == bufend ) {
			dst->head = dst->buf;
		}
	}
	
	if( overflow ) {
		dst->tail = ringbuf_nextp( dst, dst->head );
		assert( ringbuf_is_full( dst ) );
	}
	
	return dst->head;
}

void *ringbuf_memcpy_from( void *dst, ringbuf_t* src, size_t count )
{
	size_t bytes_used = ringbuf_bytes_used( src );
	if( count > bytes_used ) {
		return 0;
	}
	
	uint8_t *u8dst = dst;
	const uint8_t *bufend = ringbuf_end( src );
	size_t nwritten = 0;
	while( nwritten != count ) {
		assert( bufend > src->tail );
		size_t n = MIN( bufend - src->tail, count - nwritten );
		memcpy( u8dst + nwritten, src->tail, n );
		src->tail += n;
		nwritten += n;
		
		/* wrap ? */
		if( src->tail == bufend ) {
			src->tail = src->buf;
		}
	}
	
	assert( count + ringbuf_bytes_used( src ) == bytes_used );
	return src->tail;
}
#endif
