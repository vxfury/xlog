#ifndef __INTERNAL_XLOG_H
#define __INTERNAL_XLOG_H

#include <xlog/xlog_config.h>

/**
 * @brief Instructs the compiler that a specific variable or function is used.
 */
#if defined(__GNUC__)
#define USED __attribute__((__used__))
#else
#define USED
#endif

/**
 *  Instructs the compiler that a specific variable is deliberately unused.
 *  This can occur when reading volatile device memory or skipping arguments
 *  in a variable argument method.
 */
#if defined(__GNUC__)
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif



/** version number */
#define XLOG_VERSION_MAJOR        	2
#define XLOG_VERSION_MINOR        	3
#define XLOG_VERSION_PATCH     		2

#if ( XLOG_VERSION_MINOR > 0xF ) || ( XLOG_VERSION_PATCH > 0xF )
#error We expected minor/revision version number which not greater than 0xF.
#endif


/** xlog magic for runtime safe check */
#define XLOG_MAGIC_BUILD(a, b, c, d)	((((d) & 0xF) << 24) | (((c) & 0xF) << 16) | (((b) & 0xF) << 8) | (((a) & 0xF) << 0))
#define XLOG_MAGIC_CONTEXT			XLOG_MAGIC_BUILD('X', 'C', 'T', 'X')
#define XLOG_MAGIC_MODULE			XLOG_MAGIC_BUILD('X', 'M', 'D', 'L')
#define XLOG_MAGIC_PRINTER			XLOG_MAGIC_BUILD('X', 'P', 'R', 'T')
#define XLOG_MAGIC_PAYLOAD			XLOG_MAGIC_BUILD('X', 'P', 'L', 'D')


/** memory allocation for xlog */
#define XLOG_MALLOC(nbytes)			((nbytes) == 0 ? NULL : calloc(1, nbytes))
#define XLOG_REALLOC(ptr, nbytes)	realloc(ptr, nbytes)
#define XLOG_FREE(ptr)				free(ptr)
#define XLOG_STRDUP(str)			strdup(str)


/** internal use */
#define STRING(x)	#x
#define XSTRING(x)	STRING(x)
#define __XLOG_TRACE(...) // fprintf( stderr, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> " ), fprintf( stderr, __VA_ARGS__ ), fprintf( stderr, "\r\n" )

#define XLOG_TRACE(...)	// xlog_output_rawlog( xlog_printer_create( XLOG_PRINTER_STDERR ), NULL, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> ", "\r\n", __VA_ARGS__ )

#ifdef __cplusplus
extern "C" {
#endif

#ifdef XLOG_FEATURE_REPLACE_VSNPRINTF
int xlog_vsnprintf( char *buf, size_t size, const char *fmt, va_list args );
#define XLOG_VSNPRINTF 				xlog_vsnprintf
#else
#define XLOG_VSNPRINTF 				vsnprintf
#endif

#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
/* allocate memory and set magic for it */
void *xlog_malloc( int magic, size_t nbytes );

/* clear magic of memory and free it */
void xlog_free( void *vptr );

/* resize memory allocated, no changes on magic */
void *xlog_realloc( void *vptr, size_t nbytes );
#endif

#ifdef __cplusplus
}
#endif

#endif
