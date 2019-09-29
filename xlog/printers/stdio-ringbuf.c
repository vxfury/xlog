#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include <xlog/plugins/ringbuf.h>

#include "internal.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

#undef __XLOG_TRACE
#define __XLOG_TRACE(...) // xlog_output_rawlog( xlog_printer_create( XLOG_PRINTER_STDERR ), NULL, "TRACE: ", "\r\n", __VA_ARGS__ )

struct __ringbuf_printer_context {
	pthread_t thread_consumer;
	bool force_exit;
	ringbuf_t *rbuff;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

static void *ringbuf_consumer_main( void *arg )
{
	unsigned char buffer[2048];
	struct __ringbuf_printer_context *context = (struct __ringbuf_printer_context *)arg;
	
	if( NULL == context ) {
		return NULL;
	}
	
	bool idle_show = false;
	while( true ) {
		int length = ringbuf_copy_from( context->rbuff , buffer, sizeof( buffer ) - 1, true );
		if( length > 0 ) {
			__XLOG_TRACE( "consumer-READ: length = %d\n", length );
			buffer[length] = '\0';
			#ifndef XLOG_BENCH_NO_OUTPUT
			fprintf( stdout, "%.*s", length, buffer );
			#endif
			XLOG_STATS_UPDATE( &context->stats, BYTE, OUTPUT, length);
			idle_show = true;
		} else if( context->force_exit ) {
			__XLOG_TRACE( "Force exit." )
			return NULL;
		} else {
			if( idle_show ) {
				__XLOG_TRACE( "Consumer IDLE." );
				idle_show = false;
			}
		}
	}
	
	/* NOTREACHED */
	return NULL;
}

static struct __ringbuf_printer_context *__ringbuf_create_context( unsigned int capacity )
{
	struct __ringbuf_printer_context *context = ( struct __ringbuf_printer_context * )XLOG_MALLOC( sizeof( struct __ringbuf_printer_context ) + capacity );
	if( context ) {
		context->rbuff = ringbuf_create( capacity );
	}
	
	return context;
}

static int __ringbuf_destory_context( struct __ringbuf_printer_context *context )
{
	if( context ) {
		pthread_mutex_lock( &context->rbuff->mutex );
		context->force_exit = true;
		pthread_mutex_unlock( &context->rbuff->mutex );
		pthread_join( context->thread_consumer, NULL );
		
		ringbuf_destory( context->rbuff );
		XLOG_FREE( context );
	}
	
	return 0;
}

static int __ringbuf_append( xlog_printer_t *printer, void *data )
{
	const char *text = ( const char * )data;
	struct __ringbuf_printer_context *_ctx = ( struct __ringbuf_printer_context * )printer->context;
	ringbuf_copy_into( _ctx->rbuff, text, strlen( text ) );
	return 0;
}

static int __ringbuf_optctl( xlog_printer_t *printer UNUSED, int option, void *vptr, size_t size )
{
	switch( option ) {
		case XLOG_PRINTER_CTRL_GABICLR: {
			if( size == sizeof( int ) && vptr ) {
				*((int *)vptr) = 1;
			}
		} break;
		default: {
			return -1;
		}
	}
	return 0;
}

xlog_printer_t *xlog_printer_create_ringbuf( size_t capacity )
{
	xlog_printer_t *printer = NULL;
	struct __ringbuf_printer_context *_prt_ctx = __ringbuf_create_context( capacity );
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * )XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__ringbuf_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			return NULL;
		}
		printer->options = XLOG_PRINTER_TYPE_OPT( XLOG_PRINTER_RINGBUF );
		printer->context = ( void * )_prt_ctx;
		printer->append = __ringbuf_append;
		printer->optctl = __ringbuf_optctl;
		
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
	__ringbuf_destory_context( ( struct __ringbuf_printer_context * )printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}
