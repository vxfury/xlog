#include <xlog/xlog.h>
#include <xlog/xlog_config.h>
#include <xlog/xlog_helper.h>

#include "internal.h"

#ifdef __GNUC__
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
	
	switch( type ) {
		case XLOG_PRINTER_STDOUT: {
			printer = &stdout_printer;
		} break;
		case XLOG_PRINTER_STDERR: {
			printer = &stderr_printer;
		} break;
		case XLOG_PRINTER_FILES_BASIC: {
			va_list ap;
			va_start( ap, options );
			const char *file = va_arg( ap, const char * );
			va_end( ap );
			printer = xlog_printer_create_basic_file( file );
		} break;
		case XLOG_PRINTER_FILES_ROTATING: {
			va_list ap;
			va_start( ap, options );
			const char *file = va_arg( ap, const char * );
			size_t max_size_per_file = va_arg( ap, size_t );
			size_t max_file_to_ratating = va_arg( ap, size_t );
			va_end( ap );
			printer = xlog_printer_create_rotating_file( file, max_size_per_file, max_file_to_ratating );
		} break;
		case XLOG_PRINTER_FILES_DAILY: {
			va_list ap;
			va_start( ap, options );
			const char *file = va_arg( ap, const char * );
			va_end( ap );
			printer = xlog_printer_create_daily_file( file );
		} break;
		case XLOG_PRINTER_RINGBUF: {
			va_list ap;
			va_start( ap, options );
			size_t capacity = va_arg( ap, size_t );
			va_end( ap );
			printer = xlog_printer_create_ringbuf( capacity );
		} break;
		default: {
			printer = NULL;
		} break;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( printer ) {
		__XLOG_TRACE( "Magic after created is 0x%X.", printer->magic );
		printer->magic = XLOG_MAGIC_PRINTER;
	}
	#endif
	
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
	
	int type = XLOG_PRINTER_TYPE_GET( printer->options );
	switch( type ) {
		case XLOG_PRINTER_STDOUT:
		case XLOG_PRINTER_STDERR: {
			__XLOG_TRACE( "stdout/stderr printer, no need to destory" );
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
			__XLOG_TRACE( "unkown printer type(%d).", type );
			return EINVAL;
		} break;
	}
	
	return 0;
}
