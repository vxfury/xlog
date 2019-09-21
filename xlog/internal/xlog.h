#ifndef __INTERNAL_XLOG_H
#define __INTERNAL_XLOG_H

#include <xlog/xlog_config.h>

/** version number */
#define XLOG_VERSION_MAJOR        	2
#define XLOG_VERSION_MINOR        	3
#define XLOG_VERSION_PATCH     		2

#if ( XLOG_VERSION_MINOR > 0xF ) || ( XLOG_VERSION_PATCH > 0xF )
#error We expected minor/revision version number which not greater than 0xF.
#endif

/** internal use */
#define STRING(x)	#x
#define XSTRING(x)	STRING(x)
#define __XLOG_TRACE(...) // fprintf( stderr, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> " ), fprintf( stderr, __VA_ARGS__ ), fprintf( stderr, "\r\n" )
#define XLOG_TRACE(...)	// xlog_output_rawlog( xlog_printer_create( XLOG_PRINTER_STDERR ), NULL, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> ", "\r\n", __VA_ARGS__ )

#endif
