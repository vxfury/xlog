#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

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

/** Basic file */
struct __basic_file_printer_context {
	char *filename;
	int fd;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

static struct __basic_file_printer_context *__basic_file_create_context( const char *file )
{
	struct __basic_file_printer_context *context = ( struct __basic_file_printer_context * )XLOG_MALLOC( sizeof( struct __basic_file_printer_context ) );
	if( context ) {
		context->filename = XLOG_STRDUP( file );
		if( context->filename == NULL ) {
			__XLOG_TRACE( "Failed to duplicate filename." );
			XLOG_FREE( context );
			
			return NULL;
		}
		context->fd = open( context->filename, O_WRONLY | O_CREAT, 0644 );
		if( context->fd < 0 ) {
			__XLOG_TRACE( "Failed to open file(%s), 'cause %s.", context->filename, strerror( errno ) );
			XLOG_FREE( context->filename );
			XLOG_FREE( context );
			
			return NULL;
		}
	}
	
	return context;
}

static int __basic_file_destory_context( struct __basic_file_printer_context *context )
{
	if( context ) {
		close( context->fd );
		context->fd = -1;
		XLOG_FREE( context->filename );
		XLOG_FREE( context );
	}
	
	return 0;
}

static int __basic_file_append( xlog_printer_t *printer, void *data )
{
	const char *text = ( const char * )data;
	struct __basic_file_printer_context *_ctx = ( struct __basic_file_printer_context * )printer->context;
	int fd = _ctx->fd;
	if( fd >= 0 ) {
		size_t size = strlen( text );
		XLOG_STATS_UPDATE( &( ( struct __basic_file_printer_context * )printer->context )->stats, BYTE, OUTPUT, size );
		#ifndef XLOG_BENCH_NO_OUTPUT
		return write( fd, text, size );
		#else
		return size;
		#endif
	} else {
		__XLOG_TRACE( "Invalid file." );
	}
	return 0;
}

xlog_printer_t *xlog_printer_create_basic_file( const char *file )
{
	xlog_printer_t *printer = NULL;
	struct __basic_file_printer_context *_prt_ctx = __basic_file_create_context( file );
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * )XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__XLOG_TRACE( "Out of memory." );
			__basic_file_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			return NULL;
		}
		printer->options = XLOG_PRINTER_TYPE_OPT( XLOG_PRINTER_FILES_BASIC );
		printer->context = ( void * )_prt_ctx;
		printer->append = __basic_file_append;
		printer->optctl = NULL;
	} else {
		__XLOG_TRACE( "Failed to create file-basic context." );
	}
	
	return printer;
}

int xlog_printer_destory_basic_file( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		return EINVAL;
	}
	#endif
	
	__basic_file_destory_context( ( struct __basic_file_printer_context * )printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}
