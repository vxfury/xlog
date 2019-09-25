#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "internal/xlog.h"
#include "internal/xlog_tree.h"
#include "internal/xlog_hexdump.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

static int __filepath( char *buffer, size_t size, const char *pattern, int i )
{
	char *_ptr_dir = strrchr( pattern, '/' );
	if( _ptr_dir == NULL ) {
		_ptr_dir = strrchr( pattern, '\\' );
	}
	char *_ptr_ext = strrchr( pattern, '.' );
	
	if( _ptr_ext ) {
		if(
		    ( _ptr_dir && _ptr_ext > _ptr_dir ) // xxx/path/to/file.ext
		    || ( _ptr_dir == NULL ) // file.ext, no parent dir
		) {
			snprintf(
			    buffer, size,
			    "%.*s_%05d%s"
			    , ( int )( _ptr_ext - pattern )
			    , pattern
			    , i
			    , _ptr_ext
			);
		} else { // xxx/path/to/file.ext/
			return -1;
		}
	} else {
		// xxx/path/to/file, or file
		snprintf( buffer, size, "%s_%05d", pattern, i );
	}
	
	return 0;
}


/** Rotating files */
struct __rotating_file_printer_context {
	char *pattern_file;
	size_t max_size_per_file;
	size_t max_file_to_ratating;
	
	int current_fd;
	int current_index;
	int current_bytes;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

static struct __rotating_file_printer_context *__rotating_file_create_context( const char *file, size_t max_size_per_file, size_t max_file_to_ratating )
{
	char buffer[256] = { 0 };
	if( __filepath( buffer, sizeof( buffer ), file, 0 ) != 0 ) {
		XLOG_TRACE( "Invalid file pattern." );
		return NULL;
	}
	struct __rotating_file_printer_context *context = ( struct __rotating_file_printer_context * )XLOG_MALLOC( sizeof( struct __rotating_file_printer_context ) );
	if( context ) {
		context->pattern_file = XLOG_STRDUP( file );
		context->max_size_per_file = max_size_per_file;
		context->max_file_to_ratating = max_file_to_ratating;
		
		context->current_bytes = 0;
		context->current_index = 0;
		context->current_fd = -1;
		
		XLOG_STATS_INIT( &context->stats, XLOG_STATS_PRINTER_OPTION );
	}
	
	return context;
}

static int __rotating_file_destory_context( struct __rotating_file_printer_context *context )
{
	if( context ) {
		XLOG_FREE( context->pattern_file );
		context->pattern_file = NULL;
		XLOG_FREE( context );
	}
	
	return 0;
}

static int rotating_file_get_fd( xlog_printer_t *printer )
{
	struct __rotating_file_printer_context *context = ( struct __rotating_file_printer_context * )printer->context;
	if( context ) {
		int fd = context->current_fd;
		if( fd < 0 ) {
			char buffer[256] = { 0 };
			__filepath( buffer, sizeof( buffer ), context->pattern_file, context->current_index );
			fd = open( buffer, O_WRONLY | O_CREAT, 0644 );
			context->current_fd = fd;
			context->current_bytes = 0;
		} else if( context->current_bytes >= context->max_size_per_file ) {
			close( fd );
			if( ( ++ context->current_index ) >= context->max_file_to_ratating ) {
				context->current_index = 0;
			}
			char buffer[256] = { 0 };
			__filepath( buffer, sizeof( buffer ), context->pattern_file, context->current_index );
			fd = open( buffer, O_WRONLY | O_CREAT, 0644 );
			context->current_fd = fd;
			context->current_bytes = 0;
		}
		
		return context->current_fd;
	}
	return -1;
}

static int rotating_file_append( xlog_printer_t *printer, const char *text )
{
	int fd = rotating_file_get_fd( printer );
	struct __rotating_file_printer_context *_ctx = ( struct __rotating_file_printer_context * )printer->context;
	if( fd >= 0 ) {
		size_t size = strlen( text );
		_ctx->current_bytes += size;
		XLOG_STATS_UPDATE( &( ( struct __rotating_file_printer_context * )printer->context )->stats, BYTE, OUTPUT, size );
		#ifndef XLOG_BENCH_NO_OUTPUT
		return write( fd, text, size );
		#else
		return size;
		#endif
	}
	return 0;
}

xlog_printer_t *xlog_printer_create_rotating_file( const char *file, size_t max_size_per_file, size_t max_file_to_ratating )
{
	xlog_printer_t *printer = NULL;
	struct __rotating_file_printer_context *_prt_ctx = __rotating_file_create_context( file, max_size_per_file, max_file_to_ratating );
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * ) XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__rotating_file_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			
			return NULL;
		}
		printer->context = ( void * )_prt_ctx;
		printer->options = XLOG_PRINTER_FILES_ROTATING;
		printer->append = rotating_file_append;
		printer->optctl = NULL;
	}
	
	return printer;
}

int xlog_printer_destory_rotating_file( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		return EINVAL;
	}
	#endif
	
	__rotating_file_destory_context( ( struct __rotating_file_printer_context * )printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}