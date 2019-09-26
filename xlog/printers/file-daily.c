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

static int __now_day( void )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	struct tm tm;
	localtime_r( &tv.tv_sec, &tm );
	
	return tm.tm_mday;
}

static int __filepath( char *buffer, size_t size, const char *pattern )
{
	char *_ptr_dir = strrchr( pattern, '/' );
	if( _ptr_dir == NULL ) {
		_ptr_dir = strrchr( pattern, '\\' );
	}
	char *_ptr_ext = strrchr( pattern, '.' );
	
	#if ((defined __linux__) || (defined __FreeBSD__) || (defined __APPLE__) || (defined __unix__))
	char __date[48];
	struct timeval tv;
	gettimeofday( &tv, NULL );
	struct tm tm;
	localtime_r( &tv.tv_sec, &tm );
	snprintf(
		__date, sizeof( __date ),
		"%02d%02d_%02d%02d%02d",
		tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec
	);
	#else
	#error No implementation for this system.
	#endif
	
	if( _ptr_ext ) {
		if(
			( _ptr_dir && _ptr_ext > _ptr_dir ) // xxx/path/to/file.ext
			|| ( _ptr_dir == NULL ) // file.ext, no parent dir
		) {
			snprintf(
				buffer, size,
				"%.*s_%s%s"
				, ( int )( _ptr_ext - pattern )
				, pattern
				, __date
				, _ptr_ext
			);
		} else { // xxx/path/to/file.ext/
			return -1;
		}
	} else {
		// xxx/path/to/file, or file
		snprintf( buffer, size, "%s_%s", pattern, __date );
	}
	
	return 0;
}


/** daily files */
struct __daily_file_printer_context {
	char *pattern_file;
	
	int current_fd;
	int current_day;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

static struct __daily_file_printer_context *__daily_file_create_context( const char *file )
{
	char buffer[256] = { 0 };
	if( __filepath( buffer, sizeof( buffer ), file ) != 0 ) {
		XLOG_TRACE( "Invalid file pattern." );
		return NULL;
	}
	struct __daily_file_printer_context *context = ( struct __daily_file_printer_context * )XLOG_MALLOC( sizeof( struct __daily_file_printer_context ) );
	if( context ) {
		context->pattern_file = XLOG_STRDUP( file );
		
		context->current_day = __now_day();
		context->current_fd = -1;
		
		XLOG_STATS_INIT( &context->stats, XLOG_STATS_PRINTER_OPTION );
	}
	
	return context;
}

static int __daily_file_destory_context( struct __daily_file_printer_context *context )
{
	if( context ) {
		XLOG_FREE( context->pattern_file );
		context->pattern_file = NULL;
		XLOG_FREE( context );
	}
	
	return 0;
}

static int daily_file_get_fd( xlog_printer_t *printer )
{
	struct __daily_file_printer_context *context = ( struct __daily_file_printer_context * )printer->context;
	if( context ) {
		int fd = context->current_fd;
		if( fd < 0 ) {
			char buffer[256] = { 0 };
			__filepath( buffer, sizeof( buffer ), context->pattern_file );
			fd = open( buffer, O_WRONLY | O_CREAT, 0644 );
			context->current_fd = fd;
			context->current_day = __now_day();
		} else if( context->current_day != __now_day() ) {
			close( fd );
			char buffer[256] = { 0 };
			__filepath( buffer, sizeof( buffer ), context->pattern_file );
			fd = open( buffer, O_WRONLY | O_CREAT, 0644 );
			context->current_fd = fd;
			context->current_day = __now_day();
		}
		
		return context->current_fd;
	}
	return -1;
}

static int daily_file_append( xlog_printer_t *printer, const char *text )
{
	int fd = daily_file_get_fd( printer );
	if( fd >= 0 ) {
		size_t size = strlen( text );
		XLOG_STATS_UPDATE( &( ( struct __daily_file_printer_context * )printer->context )->stats, BYTE, OUTPUT, size );
		#ifndef XLOG_BENCH_NO_OUTPUT
		return write( fd, text, size );
		#else
		return size;
		#endif
	}
	return 0;
}

xlog_printer_t *xlog_printer_create_daily_file( const char *file )
{
	xlog_printer_t *printer = NULL;
	struct __daily_file_printer_context *_prt_ctx = __daily_file_create_context( file );
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * ) XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__daily_file_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			
			return NULL;
		}
		printer->context = ( void * )_prt_ctx;
		printer->options = XLOG_PRINTER_FILES_DAILY;
		printer->append = daily_file_append;
		printer->optctl = NULL;
	}
	
	return printer;
}

int xlog_printer_destory_daily_file( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		return EINVAL;
	}
	#endif
	
	__daily_file_destory_context( ( struct __daily_file_printer_context * )printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}