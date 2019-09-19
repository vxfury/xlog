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

#define XLOG_MAGIC_FROM_OBJECT(pobj) *(((int *)(pobj)) - 1)

/** memory allocation for xlog */
#define XLOG_MALLOC(nbytes)			((nbytes) == 0 ? NULL : calloc(1, nbytes))
#define XLOG_REALLOC(ptr, nbytes)	realloc(ptr, nbytes)
#define XLOG_FREE(ptr)				free(ptr)
#define XLOG_STRDUP(str)			strdup(str)

#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
#define XLOG_MALLOC_RUNTIME_SAFE(magic, nbytes)	((nbytes) == 0 ? NULL : xlog_malloc(magic, nbytes))
#define XLOG_REALLOC_RUNTIME_SAFE(ptr, nbytes)	xlog_realloc(ptr, nbytes)
#define XLOG_FREE_RUNTIME_SAFE(ptr)				xlog_free(ptr)
#else
#define XLOG_MALLOC_RUNTIME_SAFE(magic, nbytes)	XLOG_MALLOC(nbytes)
#define XLOG_REALLOC_RUNTIME_SAFE(ptr, nbytes)	XLOG_REALLOC(ptr, nbytes)
#define XLOG_FREE_RUNTIME_SAFE(ptr)				XLOG_FREE(ptr)
#endif


/** internal use */
#define XLOG_TRACE(...)	printf( "<%s:%d> ", __func__, __LINE__ ), printf( __VA_ARGS__ ), printf( "\n" )


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
