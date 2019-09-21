#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>
#include <xlog/xlog_payload.h>

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

/** STATIC printer: stdout(default)/stderr */
static struct __stdio_printer_context {
	pthread_mutex_t lock;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
}
stdout_printer_context = {
	.lock = PTHREAD_MUTEX_INITIALIZER,
},
stderr_printer_context = {
	.lock = PTHREAD_MUTEX_INITIALIZER,
};

static int __stdxxx_append(xlog_printer_t *printer, const char *text)
{
	return fprintf(
	    XLOG_PRINTER_TYPE_GET(printer->options) == XLOG_PRINTER_STDOUT ? stdout : stderr,
	    "%s", text
	);
}

static int __stdxxx_control(xlog_printer_t *printer, int option, void *vptr UNUSED)
{
	struct __stdio_printer_context * context = (struct __stdio_printer_context *)printer->context;
	switch(option) {
		case XLOG_PRINTER_CTRL_LOCK: {
			pthread_mutex_lock( &context->lock );
		} break;
		case XLOG_PRINTER_CTRL_UNLOCK: {
			pthread_mutex_unlock( &context->lock );
		} break;
		default: {
			return -1;
		}
	}
	return 0;
}


static xlog_printer_t stdout_printer = {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	.magic = XLOG_MAGIC_PRINTER,
	#endif
	.context = (void *)&stdout_printer_context,
	.options = XLOG_PRINTER_STDOUT,
	.append  = __stdxxx_append,
	.control = __stdxxx_control,
},
stderr_printer = {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	.magic = XLOG_MAGIC_PRINTER,
	#endif
	.context = (void *)&stderr_printer_context,
	.options = XLOG_PRINTER_STDERR,
	.append  = __stdxxx_append,
	.control = __stdxxx_control,
};

/** Basic file */
struct __basic_file_printer_context {
	char *filename;
	int fd;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
	xlog_stats_t stats;
	#endif
};

static struct __basic_file_printer_context *__basic_file_create_context(const char *file)
{
	struct __basic_file_printer_context *context = (struct __basic_file_printer_context *)XLOG_MALLOC( sizeof( struct __basic_file_printer_context ) );
	if( context ) {
		context->filename = XLOG_STRDUP( file );
		if( context->filename == NULL ) {
			XLOG_FREE( context );
			
			return NULL;
		}
		context->fd = open( context->filename, O_WRONLY );
		if( context->fd < 0 ) {
			XLOG_FREE( context->filename );
			XLOG_FREE( context );
			
			return 0;
		}
	}
	
	return context;
}

static int __basic_file_destory_context( struct __basic_file_printer_context *context )
{
	if( context ) {
		XLOG_FREE( context->filename );
		XLOG_FREE( context );
		close( context->fd );
	}
	
	return 0;
}

static int __basic_file_append(xlog_printer_t *printer, const char *text)
{
	struct __basic_file_printer_context *_ctx = (struct __basic_file_printer_context *)printer->context;
	int fd = _ctx->fd;
	if( fd >= 0 ) {
		size_t size = strlen(text);
		XLOG_STATS_UPDATE( &((struct __basic_file_printer_context *)printer->context)->stats, BYTE, OUTPUT, size );
		return write( fd, text, size );
	}
	return 0;
}

static int __basic_files_control(xlog_printer_t *printer UNUSED, int option UNUSED, void *vptr UNUSED)
{
	return 0;
}

/** Rotating files */
struct __rotating_files_printer_context {
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
static struct __rotating_files_printer_context *__rotating_files_create_context(const char *file, size_t max_size_per_file, size_t max_file_to_ratating)
{
	struct __rotating_files_printer_context *context = (struct __rotating_files_printer_context *)XLOG_MALLOC( sizeof( struct __rotating_files_printer_context ) );
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

static int __rotating_files_destory_context(struct __rotating_files_printer_context * context)
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

static int rotating_files_get_fd(xlog_printer_t *printer)
{
	struct __rotating_files_printer_context * context = (struct __rotating_files_printer_context *)printer->context;
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

static int rotating_files_append(xlog_printer_t *printer, const char *text)
{
	int fd = rotating_files_get_fd( printer );
	struct __rotating_files_printer_context *_ctx = (struct __rotating_files_printer_context *)printer->context;
	if( fd >= 0 ) {
		size_t size = strlen(text);
		_ctx->_status.offsets[_ctx->_status.current_file_id] += size;
		XLOG_STATS_UPDATE( &((struct __rotating_files_printer_context *)printer->context)->stats, BYTE, OUTPUT, size );
		return write( fd, text, size );
	}
	return 0;
}

static int rotating_files_control(xlog_printer_t *printer UNUSED, int option UNUSED, void *vptr UNUSED)
{
	return 0;
}

static xlog_printer_t *__default_printer = &stdout_printer;

/**
 * @brief  get default printer
 *
 * @return pointer to printer
 *
 */
XLOG_PUBLIC(xlog_printer_t *) xlog_printer_default( void )
{
	return __default_printer;
}

/**
 * @brief  create dynamic printer
 *
 * @param  options, options to create printer
 * @return pointer to printer
 *
 */
XLOG_PUBLIC(xlog_printer_t *) xlog_printer_create( int options, ... )
{
	xlog_printer_t *printer = NULL;
	int type = XLOG_PRINTER_TYPE_GET(options);
	
	switch( type ) {
		case XLOG_PRINTER_STDOUT: {
			printer = &stdout_printer;
		} break;
		case XLOG_PRINTER_STDERR: {
			printer = &stderr_printer;
		} break;
		case XLOG_PRINTER_FILES_BASIC: {
			va_list ap;
			va_start(ap, options);
			const char *file = va_arg( ap, const char * );
			va_end(ap);
			struct __basic_file_printer_context * _prt_ctx = __basic_file_create_context( file );
			if( _prt_ctx ) {
				printer = ( xlog_printer_t * )XLOG_MALLOC( sizeof( xlog_printer_t ) );
				if( printer == NULL ) {
					__basic_file_destory_context( _prt_ctx );
					XLOG_FREE( printer );
					break;
				}
				printer->context = (void *)_prt_ctx;
				printer->append = __basic_file_append;
				printer->control = __basic_files_control;
			}
		} break;
		case XLOG_PRINTER_FILES_ROTATING: {
			va_list ap;
			va_start(ap, options);
			printer = NULL;
			const char *file = va_arg( ap, const char * );
			size_t max_size_per_file = va_arg( ap, size_t );
			size_t max_file_to_ratating = va_arg( ap, size_t );
			va_end(ap);
			
			struct __rotating_files_printer_context * _prt_ctx = __rotating_files_create_context( file, max_size_per_file, max_file_to_ratating);
			if( _prt_ctx ) {
				printer = ( xlog_printer_t * ) XLOG_MALLOC( sizeof( xlog_printer_t ) );
				if( printer == NULL ) {
					__rotating_files_destory_context( _prt_ctx );
					_prt_ctx = NULL;
					break;
				}
				printer->context = (void *)_prt_ctx;
				printer->options = XLOG_PRINTER_FILES_ROTATING;
				printer->append = rotating_files_append;
				printer->control = rotating_files_control;
			}
		} break;
		case XLOG_PRINTER_FILES_DAILY: {
			va_list ap;
			va_start(ap, options);
			printer = NULL;
			
			va_end(ap);
		} break;
		default: {
			printer = NULL;
		} break;
	}
	
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
XLOG_PUBLIC(int) xlog_printer_destory( xlog_printer_t *printer )
{
	if( printer == NULL ) {
		return EINVAL;
	}
	
	int type = XLOG_PRINTER_TYPE_GET( printer->options );
	switch( type ) {
		case XLOG_PRINTER_STDOUT:
		case XLOG_PRINTER_STDERR: {
			XLOG_TRACE( "stdout/stderr printer, no need to destory" );
		} break;
		case XLOG_PRINTER_FILES_BASIC: {
			//
		} break;
		case XLOG_PRINTER_FILES_ROTATING: {
			__rotating_files_destory_context( (struct __rotating_files_printer_context *)printer->context );
			printer->context = NULL;
			XLOG_FREE( printer );
		} break;
		case XLOG_PRINTER_FILES_DAILY: {
			//
		} break;
		default: {
			XLOG_TRACE( "unkown printer type(%d).", type );
			return EINVAL;
		} break;
	}
	
	return 0;
}
