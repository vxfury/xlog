#ifndef __INTERNAL_XLOG_H
#define __INTERNAL_XLOG_H

#include <xlog/xlog_config.h>
#include <xlog/xlog_helper.h>

/** version number */
#define XLOG_VERSION_MAJOR        	2
#define XLOG_VERSION_MINOR        	3
#define XLOG_VERSION_PATCH     		4

#if ( XLOG_VERSION_MINOR > 0xF ) || ( XLOG_VERSION_PATCH > 0xF )
#error We expected minor/revision version number which not greater than 0xF.
#endif

/** internal use */
#define STRING(x)	#x
#define XSTRING(x)	STRING(x)
/* trace for xlog-printer and xlog_output_rawlog */
#define __XLOG_TRACE(...) // fprintf( stderr, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> " ), fprintf( stderr, __VA_ARGS__ ), fprintf( stderr, "\r\n" )
/* trace for others exclude targets of __XLOG_TRACE */
#define XLOG_TRACE(...)	// xlog_output_rawlog( xlog_printer_create( XLOG_PRINTER_STDERR ), NULL, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> ", "\r\n", __VA_ARGS__ )

#ifdef __cplusplus
extern "C" {
#endif

extern xlog_printer_t stdout_printer, stderr_printer;

xlog_printer_t *xlog_printer_create_basic_file( const char *file );
int xlog_printer_destory_basic_file( xlog_printer_t *printer );

xlog_printer_t *xlog_printer_create_rotating_file( const char *file, size_t max_size_per_file, size_t max_file_to_ratating );
int xlog_printer_destory_rotating_file( xlog_printer_t *printer );

xlog_printer_t *xlog_printer_create_daily_file( const char *file );
int xlog_printer_destory_daily_file( xlog_printer_t *printer );

xlog_printer_t *xlog_printer_create_ringbuf( size_t capacity );
int xlog_printer_destory_ringbuf( xlog_printer_t *printer );

#ifdef __cplusplus
}
#endif

#endif
