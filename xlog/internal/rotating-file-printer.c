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

/** Rotating files */
struct __rotating_file_printer_context {
	char *pattern_file;
	size_t max_size_per_file;
	size_t max_file_to_ratating;
	
	struct {
		char **filenames;
		size_t *offsets;
		int current_file_id;
		int current_fd;
	} _status;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

#define XLOG_PRINTER_FILES_ROTATING_LIMIT_COUNT_STRLEN 12
static struct __rotating_file_printer_context *__rotating_file_create_context(const char *file, size_t max_size_per_file, size_t max_file_to_ratating)
{
	struct __rotating_file_printer_context *context = (struct __rotating_file_printer_context *)XLOG_MALLOC( sizeof( struct __rotating_file_printer_context ) );
	if( context ) {
		context->pattern_file = XLOG_STRDUP( file );
		context->max_size_per_file = max_size_per_file;
		context->max_file_to_ratating = max_file_to_ratating;
		context->_status.filenames = (char **)XLOG_MALLOC( sizeof( char * ) * max_file_to_ratating );
		if( context->_status.filenames == NULL ) {
			XLOG_FREE( context );
			
			return NULL;
		}
		context->_status.offsets = (size_t *)XLOG_MALLOC( sizeof( size_t ) * max_file_to_ratating );
		if( context->_status.offsets == NULL ) {
			XLOG_FREE( context->_status.filenames );
			XLOG_FREE( context );
			
			return NULL;
		}
		memset( context->_status.offsets, 0, sizeof( size_t ) * max_file_to_ratating );
		memset( context->_status.filenames, 0, sizeof( char * ) * max_file_to_ratating );
		for( int i = 0; i < max_file_to_ratating; i ++ ) {
			size_t __namelen = strlen( file ) + XLOG_PRINTER_FILES_ROTATING_LIMIT_COUNT_STRLEN;
			char *__filename = (char *)XLOG_MALLOC( __namelen );
			if( __filename == NULL ) {
				if( i == 0 ) {
					XLOG_FREE( context->_status.filenames );
					XLOG_FREE( context );
					return NULL;
				}
				break;
			}
			memset( __filename, 0, __namelen );
			context->_status.filenames[i] = __filename;
			
			char _buffcnt[XLOG_PRINTER_FILES_ROTATING_LIMIT_COUNT_STRLEN + 1] = { 0 };
			snprintf( _buffcnt, sizeof( _buffcnt ), "%05d", i );
			/* generate filename for each log file */
			char *_ptr_dir = strrchr( file, '/' );
			if( _ptr_dir == NULL ) {
				_ptr_dir = strrchr( file, '\\' );
			}
			char *_ptr_ext = strrchr( file, '.' );
			if( _ptr_ext ) {
				if(
				   ( _ptr_dir && _ptr_ext > _ptr_dir ) // xxx/path/to/file.ext
				   || ( _ptr_dir == NULL ) // file.ext, no parent dir
				) {
					// xxx/path/to/file.ext
					strncat( __filename, file, _ptr_ext - file );
					strcat( __filename, _buffcnt );
					strcat( __filename, _ptr_ext );
				} else { // xxx/path/to/file.ext/
					assert(0);
				}
			} else {
				// xxx/path/to/file, or file
				sprintf( __filename, "%s%s", file, _buffcnt );
			}
		}
		context->_status.current_file_id = 0;
		context->_status.current_fd = -1;
		XLOG_STATS_INIT( &context->stats, XLOG_STATS_PRINTER_OPTION );
	}

	return context;
}

static int __rotating_file_destory_context(struct __rotating_file_printer_context * context)
{
	if( context ) {
		for( int i = 0; i < context->max_file_to_ratating; i ++ ) {
			XLOG_FREE( context->_status.filenames[i] );
			context->_status.filenames[i] = NULL;
		}
		XLOG_FREE( context->_status.filenames );
		context->_status.filenames = NULL;
		XLOG_FREE( context->_status.offsets );
		context->_status.offsets = NULL;
		XLOG_FREE( context->pattern_file );
		context->pattern_file = NULL;
		XLOG_FREE( context );
	}
	
	return 0;
}

static int rotating_file_get_fd(xlog_printer_t *printer)
{
	struct __rotating_file_printer_context * context = (struct __rotating_file_printer_context *)printer->context;
	if( context ) {
		int fd = context->_status.current_fd;
		if( fd < 0 ) {
			fd = open( context->_status.filenames[context->_status.current_file_id], O_WRONLY | O_CREAT, 0644 );
			context->_status.current_fd = fd;
		} else if( context->_status.offsets[context->_status.current_file_id] >= context->max_size_per_file ) {
			close( fd );
			if( ( ++ context->_status.current_file_id ) >= context->max_file_to_ratating ) {
				context->_status.current_file_id = 0;
			}
			fd = open( context->_status.filenames[context->_status.current_file_id], O_WRONLY | O_CREAT, 0644 );
			context->_status.current_fd = fd;
		}
		
		return context->_status.current_fd;
	}
	return -1;
}

static int rotating_file_append(xlog_printer_t *printer, const char *text)
{
	int fd = rotating_file_get_fd( printer );
	struct __rotating_file_printer_context *_ctx = (struct __rotating_file_printer_context *)printer->context;
	if( fd >= 0 ) {
		size_t size = strlen(text);
		_ctx->_status.offsets[_ctx->_status.current_file_id] += size;
		XLOG_STATS_UPDATE( &((struct __rotating_file_printer_context *)printer->context)->stats, BYTE, OUTPUT, size );
		return write( fd, text, size );
	}
	return 0;
}

static int rotating_file_control(xlog_printer_t *printer UNUSED, int option UNUSED, void *vptr UNUSED)
{
	return 0;
}

xlog_printer_t *xlog_printer_create_rotating_file( const char *file, size_t max_size_per_file, size_t max_file_to_ratating )
{
	xlog_printer_t *printer = NULL;
	struct __rotating_file_printer_context * _prt_ctx = __rotating_file_create_context( file, max_size_per_file, max_file_to_ratating);
	if( _prt_ctx ) {
		printer = ( xlog_printer_t * ) XLOG_MALLOC( sizeof( xlog_printer_t ) );
		if( printer == NULL ) {
			__rotating_file_destory_context( _prt_ctx );
			_prt_ctx = NULL;
			
			return NULL;
		}
		printer->context = (void *)_prt_ctx;
		printer->options = XLOG_PRINTER_FILES_ROTATING;
		printer->append = rotating_file_append;
		printer->control = rotating_file_control;
	}
	
	return printer;
}

int xlog_printer_destory_rotating_file( xlog_printer_t *printer )
{
	#if (defined XLOG_POLICY_RUNTIME_SAFE)
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		return EINVAL;
	}
	#endif
	
	__rotating_file_destory_context( (struct __rotating_file_printer_context *)printer->context );
	printer->context = NULL;
	XLOG_FREE( printer );
	
	return 0;
}