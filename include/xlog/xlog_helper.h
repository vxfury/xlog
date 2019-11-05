#ifndef __XLOG_HELPER_H
#define __XLOG_HELPER_H

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <xlog/xlog_config.h>
#include <xlog/plugins/bitops.h>
#include <xlog/plugins/autobuf.h>

#if (defined HAVE_ASM_ATOMIC_H)
#include <asm/atomic.h>
#endif

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


#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/*

When compiling for windows, we specify a specific calling convention to avoid issues
where we are being called from a project with a different default calling convention.

For windows you have 3 define options:

XLOG_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
XLOG_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
XLOG_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the XLOG_API_VISIBILITY flag to "export" the same symbols the way XLOG_EXPORT_SYMBOLS does

*/

#define XLOG_CDECL __cdecl
#define XLOG_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(XLOG_HIDE_SYMBOLS) && !defined(XLOG_IMPORT_SYMBOLS) && !defined(XLOG_EXPORT_SYMBOLS)
#define XLOG_EXPORT_SYMBOLS
#endif

#if defined(XLOG_HIDE_SYMBOLS)
#define XLOG_PUBLIC(type)   type XLOG_STDCALL
#elif defined(XLOG_EXPORT_SYMBOLS)
#define XLOG_PUBLIC(type)   __declspec(dllexport) type XLOG_STDCALL
#elif defined(XLOG_IMPORT_SYMBOLS)
#define XLOG_PUBLIC(type)   __declspec(dllimport) type XLOG_STDCALL
#endif

#else /* !__WINDOWS__ */

#define XLOG_CDECL
#define XLOG_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(XLOG_API_VISIBILITY)
#define XLOG_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define XLOG_PUBLIC(type) type
#endif
#endif


#include <xlog/xlog_config.h>

/** memory allocation for xlog */
#define XLOG_MALLOC(nbytes)			((nbytes) == 0 ? NULL : calloc(1, nbytes))
#define XLOG_REALLOC(ptr, nbytes)	realloc(ptr, nbytes)
#define XLOG_FREE(ptr)				free(ptr)
#define XLOG_STRDUP(str)			strdup(str)
#define XLOG_STRNDUP(str, n)		strndup(str, n)

/** basic macros */
#define XLOG_MIN(a, b)				((a) <= (b) ? (a) : (b))
#define XLOG_MAX(a, b)				((a) >= (b) ? (a) : (b))
#define XLOG_ARRAY_SIZE(arr) 		sizeof(arr)/sizeof(arr[0])
#define XLOG_ALIGN_DOWN(size, align)	(size & (~(align-1)))
#define XLOG_ALIGN_UP(size, align)		((align+size-1) & (~(align-1)))


/** xlog level */
#define __XLOG_LEVEL_SILENT			(0)
#define __XLOG_LEVEL_FATAL 			(1)
#define __XLOG_LEVEL_ERROR 			(2)
#define __XLOG_LEVEL_WARN 			(3)
#define __XLOG_LEVEL_INFO 			(4)
#define __XLOG_LEVEL_DEBUG 			(5)
#define __XLOG_LEVEL_VERBOSE 		(6)
#define XLOG_LIMIT_LEVEL_NUMBER 	(7)

#define XLOG_IF_DROP_LEVEL(level, limit)	((level) > (limit))
#define XLOG_IF_LOWER_LEVEL(level, limit)	((level) < (limit))
#define XLOG_IF_LEGAL_LEVEL(level)			(((level) >= __XLOG_LEVEL_SILENT) && ((level) <= __XLOG_LEVEL_VERBOSE))
#define XLOG_IF_NOT_SILENT_LEVEL(level)		(((level) > __XLOG_LEVEL_SILENT) && ((level) <= __XLOG_LEVEL_VERBOSE))


/** xlog structures & types */
typedef struct {
	int option;
	#if (defined HAVE_ASM_ATOMIC_H)
	atomic_t *data;
	#else
	unsigned int *data;
	#endif
} xlog_stats_t;

typedef struct {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	int magic;
	#endif
	pthread_mutex_t lock;
	
	int level;
	char *name;
	void *context;
	#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_MODULE)
	xlog_stats_t stats;
	#endif
} xlog_module_t;

typedef struct __xlog_printer {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	int magic;
	#endif
	void *context;
	int options;
	int ( *append )( struct __xlog_printer *printer, void *data );
	int ( *optctl )( struct __xlog_printer *printer, int option, void *vptr, size_t size );
} xlog_printer_t;

typedef struct xlog_level_attr_tag {
	int format;
	const char *time_prefix, *time_suffix;
	const char *class_prefix, *class_suffix;
	const char *body_prefix, *body_suffix;
} xlog_level_attr_t;

typedef struct {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	int magic;
	#endif
	pthread_mutex_t lock;
	int options;
	char *savepath;
	size_t initial_size;
	#if (defined XLOG_FEATURE_ENABLE_DYNAMIC_DEFAULT_AUTOBUF_SIZE)
	size_t *size_votes;
	#endif
	xlog_level_attr_t attributes[XLOG_LIMIT_LEVEL_NUMBER];
	xlog_module_t *module;
	#ifdef XLOG_FEATURE_ENABLE_STATS
	xlog_stats_t stats;
	#endif
} xlog_t;


/** xlog printer control options */
#define XLOG_PRINTER_CTRL_LOCK		0
#define XLOG_PRINTER_CTRL_UNLOCK	1
#define XLOG_PRINTER_CTRL_FLUSH		2
#define XLOG_PRINTER_CTRL_NOBUFF	3
#define XLOG_PRINTER_CTRL_GABICLR	4

/** printer for xlog */
#define XLOG_PRINTER_TYPE_OPT(type)		BITS_MASK_K(0, 4, type)
#define XLOG_PRINTER_TYPE_GET(options)	CHK_BITS(options, 0, 4)
#define XLOG_PRINTER_BUFF_OPT(type)		BITS_MASK_K(4, 8, type)
#define XLOG_PRINTER_BUFF_GET(options)	CHK_BITS(options, 4, 8)

/** statistics for xlog */
#define XLOG_STATS_MAJOR_BYTE		0
#define XLOG_STATS_MAJOR_REQUEST	1
#define XLOG_STATS_MAJOR_MIN		XLOG_STATS_MAJOR_BYTE
#define XLOG_STATS_MAJOR_MAX		XLOG_STATS_MAJOR_REQUEST

#define XLOG_STATS_MINOR_INPUT		8
#define XLOG_STATS_MINOR_OUTPUT		9
#define XLOG_STATS_MINOR_DROPPED	10
#define XLOG_STATS_MINOR_FILTERED	11
#define XLOG_STATS_MINOR_MIN		XLOG_STATS_MINOR_INPUT
#define XLOG_STATS_MINOR_MAX		XLOG_STATS_MINOR_FILTERED

#define XLOG_STATS_OBYTE			BIT_MASK(XLOG_STATS_MAJOR_BYTE)
#define XLOG_STATS_OREQUEST			BIT_MASK(XLOG_STATS_MAJOR_REQUEST)
#define XLOG_STATS_OINPUT			BIT_MASK(XLOG_STATS_MINOR_INPUT)
#define XLOG_STATS_OOUTPUT			BIT_MASK(XLOG_STATS_MINOR_OUTPUT)
#define XLOG_STATS_ODROPPED			BIT_MASK(XLOG_STATS_MINOR_DROPPED)
#define XLOG_STATS_OFILTERED		BIT_MASK(XLOG_STATS_MINOR_FILTERED)

#define XLOG_STATS_LENGTH_MAX		(XLOG_STATS_MINOR_MAX - XLOG_STATS_MINOR_MIN + 1) * (XLOG_STATS_MAJOR_MAX - XLOG_STATS_MAJOR_MIN + 1)
#define XLOG_STATS_LENGTH(option)	(BITS_COUNT(CHK_BITS(option, XLOG_STATS_MAJOR_MIN, XLOG_STATS_MAJOR_MAX + 1)) * BITS_COUNT(CHK_BITS(option, XLOG_STATS_MINOR_MIN, XLOG_STATS_MINOR_MAX + 1)))
#define XLOG_STATS_OFFSET(option, major, minor) (BITS_COUNT(CHK_BITS(option, XLOG_STATS_MAJOR_MIN, XLOG_STATS_MAJOR_MAX + 1)) * ((minor) - XLOG_STATS_MINOR_MIN) + ((major) - XLOG_STATS_MAJOR_MIN))
#define XLOG_STATS_ABICHK(option, major, minor)	((option) & (BIT_MASK(major) | BIT_MASK(minor)))

#define XLOG_STATS_CONTEXT_OPTION	(XLOG_STATS_OBYTE | XLOG_STATS_OREQUEST | XLOG_STATS_OINPUT | XLOG_STATS_OOUTPUT)
#define XLOG_STATS_MODULE_OPTION	(XLOG_STATS_OBYTE | XLOG_STATS_OREQUEST | XLOG_STATS_OINPUT | XLOG_STATS_OOUTPUT | XLOG_STATS_ODROPPED)
#define XLOG_STATS_PRINTER_OPTION	(XLOG_STATS_OBYTE | XLOG_STATS_OREQUEST | XLOG_STATS_OINPUT | XLOG_STATS_OOUTPUT | XLOG_STATS_ODROPPED)

#ifdef XLOG_FEATURE_ENABLE_STATS

#if (defined HAVE_ASM_ATOMIC_H)

#define XLOG_STATS_INIT(pstats, options)	do { \
	size_t msize = XLOG_STATS_LENGTH( options ) * sizeof( atomic_t ); \
	(pstats)->option = options; \
	if( msize > 0 ) { \
		(pstats)->data = (atomic_t *)XLOG_MALLOC( msize ); \
		if( (pstats)->data ) { memset( (pstats)->data, 0, msize ); } \
	} \
} while(0)

#define XLOG_STATS_FINI(pstats)	do { \
	if( (pstats)->data ) { XLOG_FREE( (pstats)->data ); (pstats)->data = NULL; } \
	(pstats)->option = 0; \
} while(0)

#define XLOG_STATS_CLEAR(pstats)	if( (pstats)->data ) { \
	size_t length = XLOG_STATS_LENGTH(options); \
	for( int i = 0; i < length; i ++ ) { \
		atomic_set( (pstats)->data + i, 0 ); \
	} \
}

#define XLOG_STATS_UPDATE(pstats, major, minor, inc)	if(XLOG_STATS_ABICHK((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)) { \
	atomic_add( (pstats)->data + XLOG_STATS_OFFSET((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor), inc ); \
}

#define XLOG_STATS_CLEAR_FILED(pstats, major, minor, value)	if(XLOG_STATS_ABICHK((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)) { \
	atomic_set( (pstats)->data + XLOG_STATS_OFFSET((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor), value ); \
}

#else

#define XLOG_STATS_INIT(pstats, options)	do { \
	size_t msize = XLOG_STATS_LENGTH( options ) * sizeof( unsigned int ); \
	(pstats)->option = options; \
	if( msize > 0 ) { \
		(pstats)->data = (unsigned int *)XLOG_MALLOC( msize ); \
		if( (pstats)->data ) { memset( (pstats)->data, 0, msize ); } \
	} \
} while(0)

#define XLOG_STATS_FINI(pstats)	do { \
	if( (pstats)->data ) { XLOG_FREE( (pstats)->data ); (pstats)->data = NULL; } \
	(pstats)->option = 0; \
} while(0)

#define XLOG_STATS_CLEAR(pstats)	if( (pstats)->data ) { \
	memset( (pstats)->data, 0, XLOG_STATS_LENGTH(options) * sizeof( unsigned int ) ); \
}

#define XLOG_STATS_UPDATE(pstats, major, minor, inc)	if(XLOG_STATS_ABICHK((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)) { \
	(pstats)->data[XLOG_STATS_OFFSET((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)] += (inc); \
}

#define XLOG_STATS_CLEAR_FILED(pstats, major, minor, value)	if(XLOG_STATS_ABICHK((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)) { \
	(pstats)->data[XLOG_STATS_OFFSET((pstats)->option, XLOG_STATS_MAJOR_##major, XLOG_STATS_MINOR_##minor)] = (value); \
}

#endif

#else

#define XLOG_STATS_INIT(pstats, options)
#define XLOG_STATS_FINI(pstats)
#define XLOG_STATS_CLEAR(pstats)
#define XLOG_STATS_UPDATE(pstats, major, minor, inc)
#define XLOG_STATS_CLEAR_FILED(pstats, major, minor, value)

#endif


#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
/** xlog magic for runtime safe check */
#define XLOG_MAGIC_BUILD(a, b, c, d)	((((d) & 0xF) << 24) | (((c) & 0xF) << 16) | (((b) & 0xF) << 8) | (((a) & 0xF) << 0))
#define XLOG_MAGIC_CONTEXT			XLOG_MAGIC_BUILD('X', 'C', 'T', 'X')
#define XLOG_MAGIC_MODULE			XLOG_MAGIC_BUILD('X', 'M', 'D', 'L')
#define XLOG_MAGIC_PRINTER			XLOG_MAGIC_BUILD('X', 'P', 'R', 'T')
#endif


/** Payload ID */
#define XLOG_PAYLOAD_ID_AUTO			0	/* Unspecified payload */
#define XLOG_PAYLOAD_ID_TEXT			1	/* Text payload */
#define XLOG_PAYLOAD_ID_BINARY			2	/* Binary payload */
#define XLOG_PAYLOAD_ID_LOG_TIME		3	/* Time payload */
#define XLOG_PAYLOAD_ID_LOG_CLASS		4	/* Log class payload(level(int) + layer-path(variable-length string)) */
#define XLOG_PAYLOAD_ID_LOG_POINT		5	/* Source location payload */
#define XLOG_PAYLOAD_ID_LOG_TASK		6	/* Task info payload, PID + TID and it's name */
#define XLOG_PAYLOAD_ID_LOG_BODY		7	/* Log body payload */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  print TEXT compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) _xlog_printer_print_TEXT(
	const autobuf_t *autobuf, xlog_printer_t *printer
);

/**
 * @brief  print BINARY compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) _xlog_printer_print_BINARY(
	const autobuf_t *autobuf, xlog_printer_t *printer
);

#ifdef __cplusplus
}
#endif

#endif
