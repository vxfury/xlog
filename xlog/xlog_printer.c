#include <xlog/xlog.h>
#include <xlog/xlog_config.h>
#include <xlog/xlog_helper.h>
#include <xlog/plugins/ringbuf.h>
#include <xlog/plugins/autobuf.h>
#include <xlog/plugins/hexdump.h>

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

static xlog_printer_t *__default_printer = &stdout_printer;

/**
 * @brief  get default printer
 *
 * @return pointer to printer
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_default( void )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( __default_printer->magic != XLOG_MAGIC_PRINTER ) {
		__XLOG_TRACE( "Default printer has been closed, switch to stdout." );
		__default_printer = &stdout_printer;
	}
	#endif
	return __default_printer;
}

/**
 * @brief  set default printer
 *
 * @param  printer, printer you'd like be the default
 *
 * @return pointer to default printer.
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_set_default( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		__XLOG_TRACE( "Invalid printer." );
		return __default_printer;
	}
	#endif
	
	if( printer->append ) {
		__XLOG_TRACE( "Default printer has changed." );
		__default_printer = printer;
	}
	
	__XLOG_TRACE( "Invalid printer" );
	return NULL;
}


struct __printer_ringbuf_context {
	bool force_exit;
	pthread_t thread_consumer;
	ringbuf_t *rbuff;
	xlog_printer_t *printer;
};

static void *__printer_ringbuf_consumer( void *arg )
{
	assert( arg );
	struct __printer_ringbuf_context *context = ( struct __printer_ringbuf_context * )arg;
	autobuf_t *autobuf = NULL;
	bool idle_show = false;
	while( true ) {
		int length = ringbuf_copy_from( context->rbuff , &autobuf, sizeof( autobuf_t * ), true );
		if( length > 0 ) {
			__XLOG_TRACE( "consumer-READ: length = %d\n", length );
			_xlog_printer_print_TEXT( autobuf, context->printer );
			autobuf_destory( &autobuf );
			idle_show = true;
		} else if( context->force_exit ) {
			__XLOG_TRACE( "Force exit." );
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

static struct __printer_ringbuf_context *__buffering_context_create_ringbuf( size_t capacity, xlog_printer_t *printer )
{
	XLOG_ASSERT( printer );
	XLOG_ASSERT( capacity > 0 );
	struct __printer_ringbuf_context *bufctx = ( struct __printer_ringbuf_context * )XLOG_MALLOC( sizeof( struct __printer_ringbuf_context ) );
	if( bufctx ) {
		bufctx->rbuff = ringbuf_create( capacity );
		if( bufctx->rbuff == NULL ) {
			XLOG_FREE( bufctx );
			__XLOG_TRACE( "Failed to create ring-bufffer" );
			return NULL;
		}
		if( pthread_create( &bufctx->thread_consumer, NULL, __printer_ringbuf_consumer, bufctx ) != 0 ) {
			__XLOG_TRACE( "Failed to start ringbuf consumer thread." );
			ringbuf_destory( bufctx->rbuff );
			XLOG_FREE( bufctx );
			
			return NULL;
		}
		bufctx->printer = printer;
	}
	
	return bufctx;
}

static int __buffering_context_destory_ringbuf( struct __printer_ringbuf_context *bufctx )
{
	assert( bufctx );
	pthread_mutex_lock( &bufctx->rbuff->mutex );
	bufctx->force_exit = true;
	pthread_mutex_unlock( &bufctx->rbuff->mutex );
	pthread_join( bufctx->thread_consumer, NULL );
	ringbuf_destory( bufctx->rbuff );
	bufctx->rbuff = NULL;
	XLOG_FREE( bufctx );
	
	return 0;
}

static int __buffering_printer_append( xlog_printer_t *printer, void *data )
{
	int buff_type = XLOG_PRINTER_BUFF_GET( printer->options );
	autobuf_t **payload = (autobuf_t **)data;
	switch( buff_type ) {
		case XLOG_PRINTER_BUFF_NONE: {
			__XLOG_TRACE( "Non-Buffing appending" );
			void *_ptr = autobuf_data_vptr( *payload );
			printer->append( printer, _ptr );
		} break;
		case XLOG_PRINTER_BUFF_NCPYRBUF: {
			__XLOG_TRACE( "No-Copy ring-buffer appending" );
			struct __printer_ringbuf_context *bufctx = ( struct __printer_ringbuf_context * )printer->context;
			ringbuf_copy_into( bufctx->rbuff, payload, sizeof( autobuf_t * ) );
			int length = (*payload)->offset;
			*payload = NULL;
			return length;
		} break;
		case XLOG_PRINTER_BUFF_RINGBUF: {
			__XLOG_TRACE( "Ring-buffer appending" );
			struct __printer_ringbuf_context *bufctx = ( struct __printer_ringbuf_context * )printer->context;
			void *_ptr = autobuf_data_vptr( *payload );
			size_t _len = (*payload)->offset;
			ringbuf_copy_into( bufctx->rbuff, _ptr, _len );
			
			return _len;
		} break;
		default: {
			XLOG_ASSERT( 0 );
		} break;
	}
	
	return 0;
}

static int __buffering_printer_optctl( xlog_printer_t *printer, int option, void *vptr, size_t size )
{
	XLOG_ASSERT( printer );
	struct __printer_ringbuf_context *bufctx = ( struct __printer_ringbuf_context * )printer->context;
	if( bufctx && bufctx->printer && bufctx->printer->optctl ) {
		return bufctx->printer->optctl( printer, option, vptr, size );
	}
	
	return -1;
}

static xlog_printer_t *__buffering_printer_create( size_t capacity, xlog_printer_t *printer )
{
	xlog_printer_t *printer_ringbuf = XLOG_MALLOC( sizeof( xlog_printer_t ) );
	if( printer_ringbuf ) {
		struct __printer_ringbuf_context *bufctx = __buffering_context_create_ringbuf( capacity, printer );
		if( bufctx == NULL ) {
			__XLOG_TRACE( "Failed to create buffering context." );
			XLOG_FREE( printer_ringbuf );
			
			return NULL;
		}
		
		printer_ringbuf->context = bufctx;
		printer_ringbuf->append = __buffering_printer_append;
		printer_ringbuf->optctl = __buffering_printer_optctl;
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		printer_ringbuf->magic = XLOG_MAGIC_PRINTER;
		#endif
	}
	
	return printer_ringbuf;
}


/**
 * @brief  create dynamic printer
 *
 * @param  options, options to create printer
 * @return pointer to printer
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_create( int options, ... )
{
	xlog_printer_t *printer = NULL;
	int type = XLOG_PRINTER_TYPE_GET( options );
	int buff_type = XLOG_PRINTER_BUFF_GET( options );
	__XLOG_TRACE( "options = 0x%X, type = %d, buffering = %d", options, type, buff_type );
	
	va_list ap;
	va_start( ap, options );
	switch( type ) {
		case XLOG_PRINTER_STDOUT: {
			printer = &stdout_printer;
		} break;
		case XLOG_PRINTER_STDERR: {
			printer = &stderr_printer;
		} break;
		case XLOG_PRINTER_FILES_BASIC: {
			const char *file = va_arg( ap, const char * );
			printer = xlog_printer_create_basic_file( file );
		} break;
		case XLOG_PRINTER_FILES_ROTATING: {
			const char *file = va_arg( ap, const char * );
			size_t max_size_per_file = va_arg( ap, size_t );
			size_t max_file_to_ratating = va_arg( ap, size_t );
			printer = xlog_printer_create_rotating_file( file, max_size_per_file, max_file_to_ratating );
		} break;
		case XLOG_PRINTER_FILES_DAILY: {
			const char *file = va_arg( ap, const char * );
			printer = xlog_printer_create_daily_file( file );
		} break;
		case XLOG_PRINTER_RINGBUF: {
			size_t capacity = va_arg( ap, size_t );
			printer = xlog_printer_create_ringbuf( capacity );
		} break;
		default: {
			printer = NULL;
		} break;
	}
	
	if( printer ) {
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		__XLOG_TRACE( "Magic after created is 0x%X.", printer->magic );
		printer->magic = XLOG_MAGIC_PRINTER;
		#endif
		switch( buff_type ) {
			case XLOG_PRINTER_BUFF_NCPYRBUF:
			case XLOG_PRINTER_BUFF_RINGBUF: {
				size_t rb_capacity = va_arg( ap, size_t );
				xlog_printer_t *buffprinter = __buffering_printer_create( rb_capacity, printer );
				if( buffprinter ) {
					#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
					buffprinter->magic = XLOG_MAGIC_PRINTER;
					#endif
					buffprinter->options = XLOG_PRINTER_BUFF_NCPYRBUF;
					printer = buffprinter;
				} else {
					__XLOG_TRACE( "Failed to create buffering-printer." );
				}
			} break;
		}
	} else {
		__XLOG_TRACE( "Failed to create printer." );
	}
	va_end( ap );
	__XLOG_TRACE( "Succeed to create printer." );
	
	return printer;
}

/**
 * @brief  destory created printer
 *
 * @param  printer, pointer to created printer
 * @return error code
 *
 * @note   you MUST call to destory printer dynamically created.
 *
 */
XLOG_PUBLIC( int ) xlog_printer_destory( xlog_printer_t *printer )
{
	if( printer == NULL ) {
		return EINVAL;
	}
	int buff_type = XLOG_PRINTER_BUFF_GET( printer->options );
	__XLOG_TRACE( "buffering = %d.", buff_type );
	switch( buff_type ) {
		case XLOG_PRINTER_BUFF_NCPYRBUF:
		case XLOG_PRINTER_BUFF_RINGBUF: {
			__XLOG_TRACE( "Destory buffering context." );
			struct __printer_ringbuf_context *bufctx = (struct __printer_ringbuf_context *)printer->context;
			printer = bufctx->printer;
			__buffering_context_destory_ringbuf( bufctx );
			bufctx->printer = NULL;
		} break;
	}
	
	int type = XLOG_PRINTER_TYPE_GET( printer->options );
	__XLOG_TRACE( "type = %d.", type );
	switch( type ) {
		case XLOG_PRINTER_STDOUT:
		case XLOG_PRINTER_STDERR: {
			__XLOG_TRACE( "stdout/stderr printer, no need to destory." );
		} break;
		case XLOG_PRINTER_FILES_BASIC: {
			xlog_printer_destory_basic_file( printer );
		} break;
		case XLOG_PRINTER_FILES_ROTATING: {
			xlog_printer_destory_rotating_file( printer );
		} break;
		case XLOG_PRINTER_FILES_DAILY: {
			xlog_printer_destory_daily_file( printer );
		} break;
		case XLOG_PRINTER_RINGBUF: {
			xlog_printer_destory_ringbuf( printer );
		} break;
		default: {
			__XLOG_TRACE( "unkown printer type(0x%X).", type );
			return EINVAL;
		} break;
	}
	
	return 0;
}

/**
 * @brief  print TEXT compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) _xlog_printer_print_TEXT(
	const autobuf_t *autobuf, xlog_printer_t *printer
)
{
	XLOG_ASSERT( autobuf );
	XLOG_ASSERT( AUTOBUF_TEXT_COMPATIBLE( autobuf->options ) );
	
	return printer->append( printer, ( void * )autobuf_data_vptr( autobuf ) );
}

static void hexdump_printline( uintmax_t cursor, const char *dumpline, void *arg )
{
	xlog_printer_t *printer = ( xlog_printer_t * )arg;
	
	char buffer[128];
	snprintf(
		buffer, sizeof( buffer ),
		"%5jx%03jx  %s\n", cursor >> 12, cursor & 0xFFF, dumpline
	);
	printer->append( printer, buffer );
}

static int hexdump_memory_readline( const void *addr, off_t offset, void *buffer, size_t size )
{
	memcpy( buffer, ( char * )addr + offset, size );
	
	return size;
}

/**
 * @brief  print BINARY compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) _xlog_printer_print_BINARY(
	const autobuf_t *autobuf, xlog_printer_t *printer
)
{
	XLOG_ASSERT( autobuf );
	XLOG_ASSERT( printer );
	
	hexdump_options_t options = {
		.start = 0,
		.end = -1,
		.columns = 16,
		.groupsize = 8,
		.use_formatting = false,
	};
	options.end = options.start + autobuf->offset;
	if( printer->optctl ) {
		printer->optctl( printer, XLOG_PRINTER_CTRL_LOCK, NULL, 0 );
	}
	int length = __hexdump( autobuf_data_vptr( autobuf ), &options, hexdump_memory_readline, hexdump_printline, printer );
	if( printer->optctl ) {
		printer->optctl( printer, XLOG_PRINTER_CTRL_UNLOCK, NULL, 0 );
	}
	
	return length;
}
