#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "internal/xlog.h"
#include "ringbuf/ringbuf.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

struct __ringbuf_printer_context {
	pthread_mutex_t mutex;
	pthread_t thread_consumer;
	ringbuf_t ringbuf;
};

static struct __ringbuf_printer_context *__ringbuf_create_context( size_t capacity )
{
	struct __ringbuf_printer_context *context = ( struct __ringbuf_printer_context * )XLOG_MALLOC( sizeof( struct __ringbuf_printer_context ) );
	if( context ) {
		context->ringbuf = ringbuf_new( capacity );
		if( context->ringbuf == NULL ) {
			XLOG_FREE( context );
			
			return NULL;
		}
		pthread_mutex_init( &context->mutex, NULL );
	}
	
	return context;
}

static int __ringbuf_destory_context( struct __ringbuf_printer_context *context )
{
	if( context ) {
		ringbuf_free( &context->ringbuf );
		XLOG_FREE( context );
	}
	
	return 0;
}

static int __ringbuf_append( xlog_printer_t *printer, const char *text )
{
	struct __ringbuf_printer_context *_ctx = ( struct __ringbuf_printer_context * )printer->context;
	pthread_mutex_lock( &_ctx->mutex );
	ringbuf_memcpy_into( _ctx->ringbuf , text, strlen( text ) );
	pthread_mutex_unlock( &_ctx->mutex );
	return 0;
}

static int __ringbuf_control( xlog_printer_t *printer UNUSED, int option UNUSED, void *vptr UNUSED )
{
	return 0;
}

static void *ringbuf_consumer_main( void *arg )
{
	static char buffer[2048];
	struct __ringbuf_printer_context *context = (struct __ringbuf_printer_context *)arg;
	
	if( NULL == context ) {
		return NULL;
	}
	
	for( ;; ) {
		pthread_mutex_lock( &context->mutex );
		if( !ringbuf_is_empty( context->ringbuf ) ) {
			int length = XLOG_MIN( 2048, ringbuf_bytes_used( context->ringbuf ) );
			ringbuf_memcpy_from( ( void * )buffer, context->ringbuf, length );
			XLOG_STATS_UPDATE( &context->stats, BYTE, OUTPUT, length);
			fprintf( stdout, "%.*s", length, buffer );
		}
		pthread_mutex_unlock( &context->mutex );
	}
	
	/* NOTREACHED */
	return NULL;
}

xlog_printer_t *xlog_printer_create_ringbuf( const char *file )
{
	xlog_printer_t *printer = NULL;
	struct __ringbuf_printer_context *_prt_ctx = __ringbuf_create_context( file );
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * )XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__ringbuf_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			return NULL;
		}
		printer->context = ( void * )_prt_ctx;
		printer->append = __ringbuf_append;
		printer->control = __ringbuf_control;
		
		if( 0 == pthread_create( &_prt_ctx->thread_consumer, NULL, ringbuf_consumer_main, _prt_ctx ) ) {
			__XLOG_TRACE( "Successed to start ringbuf consumer thread." );
		} else {
			__XLOG_TRACE( "Failed to start ringbuf consumer thread." );
			__ringbuf_destory_context( _prt_ctx );
			XLOG_FREE( printer );
			
			return NULL;
		}
	}
	
	return printer;
}

int xlog_printer_destory_ringbuf( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		return EINVAL;
	}
	#endif
	pthread_join( ( ( struct __ringbuf_printer_context * )printer->context )->thread_consumer, NULL );
	__ringbuf_destory_context( ( struct __ringbuf_printer_context * )printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}

