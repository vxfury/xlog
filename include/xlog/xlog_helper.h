#ifndef __XLOG_HELPER_H
#define __XLOG_HELPER_H

#ifdef __GNUC__
// #pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
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

/** basic macros */
#define XLOG_MIN(a, b)				((a) <= (b) ? (a) : (b))
#define XLOG_MAX(a, b)				((a) >= (b) ? (a) : (b))
#define XLOG_ARRAY_SIZE(arr) 		sizeof(arr)/sizeof(arr[0])
#define XLOG_ALIGN_DOWN(size, align)	(size & (~(align-1)))
#define XLOG_ALIGN_UP(size, align)		((align+size-1) & (~(align-1)))

/** bit operations for xlog development */
#ifndef BITS_PER_LONG
#error BITS_PER_LONG must be defined.
#endif

#define DIV_ROUND_UP(n,d)		(((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_LONG)

/** bit mask */
#define BIT_WORD(nr)			((nr) / BITS_PER_LONG)
#define BIT_MASK(nr)			(1UL << ((nr) & (BITS_PER_LONG - 1)))
#define BITS_MASK(lo, hi)		((BIT_MASK((hi) - (lo)) - 1) << ((lo) & (BITS_PER_LONG - 1)))
#define BITS_MASK_K(lo, hi, k)	(((BIT_MASK((hi) - (lo)) - 1) & (k)) << ((lo) & (BITS_PER_LONG - 1)))

/** bit(s) operation */
#define SET_BIT(v, nr)			(v |= BIT_MASK(nr))
#define CLR_BIT(v, nr)			(v &= ~BIT_MASK(nr))
#define REV_BIT(v, nr)			(v ^= ~BIT_MASK(nr))
#define CHK_BIT(v, nr)			(v & BIT_MASK(nr))
#define GET_BIT(v, nr)			(!!CHK_BIT(v, nr))
#define SET_BITS(v, lo, hi)		(v |= BITS_MASK(lo, hi))
#define CLR_BITS(v, lo, hi)		(v &= ~BITS_MASK(lo, hi))
#define REV_BITS(v, lo, hi)		(v ^= ~BITS_MASK(lo, hi))
#define CHK_BITS(v, lo, hi)		(v & BITS_MASK(lo, hi))
#define GET_BITS(v, lo, hi)		(CHK_BITS(v, lo, hi) >> ((lo) & (BITS_PER_LONG - 1)))
#define SETV_BITS(v, lo, hi, k)	(CLR_BITS(v, lo, hi), v |= BITS_MASK_K(lo, hi, k))

/** bit algorithms */
#define CLR_LoBIT(v)			((v) & ((v) - 1))
#define CLR_LoBITS(v)			((v) & ((v) + 1))
#define SET_LoBIT(v)			((v) | ((v) + 1))
#define SET_LoBITS(v)			((v) | ((v) - 1))
#define GET_LoBIT(v)			(v) & (~(v) + 1)

#define IS_POWER_OF_2(v)		(CLR_LoBIT(v) == 0)
#define IS_POWER_OF_4(v)		(CLR_LoBIT(v) == 0 && ((v) & 0x55555555))

#define AVERAGE(x, y)			(((x) & (y)) + (((x) ^ (y)) >> 1))

/** count how many bits set */
static inline unsigned long BITS_COUNT( unsigned long x )
{
	#if BITS_PER_LONG == 32
	x = ( x & 0x55555555 ) + ( ( x & 0xaaaaaaaa ) >> 1 );
	x = ( x & 0x33333333 ) + ( ( x & 0xcccccccc ) >> 2 );
	x = ( x & 0x0f0f0f0f ) + ( ( x & 0xf0f0f0f0 ) >> 4 );
	x = ( x & 0x00ff00ff ) + ( ( x & 0xff00ff00 ) >> 8 );
	x = ( x & 0x0000ffff ) + ( ( x & 0xffff0000 ) >> 16 );
	#elif BITS_PER_LONG == 64
	x = ( x & 0x5555555555555555 ) + ( ( x & 0xaaaaaaaaaaaaaaaa ) >> 1 );
	x = ( x & 0x3333333333333333 ) + ( ( x & 0xcccccccc33333333 ) >> 2 );
	x = ( x & 0x0f0f0f0f0f0f0f0f ) + ( ( x & 0xf0f0f0f0f0f0f0f0 ) >> 4 );
	x = ( x & 0x00ff00ff00ff00ff ) + ( ( x & 0xff00ff00ff00ff00 ) >> 8 );
	x = ( x & 0x0000ffff0000ffff ) + ( ( x & 0xffff0000ffff0000 ) >> 16 );
	x = ( x & 0x00000000ffffffff ) + ( ( x & 0xffffffff00000000 ) >> 32 );
	#endif
	
	return x;
}

/** get position of first clear bit */
static inline int BITS_FLS( unsigned long v )
{
	int rv = BITS_PER_LONG;
	if( 0 == v ) {
		return 0;
	}
	
	#define __X(rv, x, mask, bits) if(0 == ((x) & (mask))) {(x) <<= (bits); rv -= (bits); }
	#if BITS_PER_LONG == 64
	__X( rv, v, 0xFFFFFFFF00000000u, 32 );
	#endif
	__X( rv, v, 0xFFFF0000u, 16 );
	__X( rv, v, 0xFF000000u,  8 );
	__X( rv, v, 0xF0000000u,  4 );
	__X( rv, v, 0xc0000000u,  2 );
	__X( rv, v, 0x80000000u,  1 );
	#undef __X
	
	return rv;
}

/** get position of first set bit */
static inline int BITS_FFS( unsigned long v )
{
	int rv = 1;
	if( 0 == v ) {
		return 0;
	}
	#define __X(rv, x, mask, bits) if(0 == ((x) & (mask))) {(x) >>= (bits); rv += (bits); }
	#if BITS_PER_LONG == 64
	__X( rv, v, 0xFFFFFFFF, 32 );
	#endif
	__X( rv, v, 0x0000FFFF, 16 );
	__X( rv, v, 0x000000FF,  8 );
	__X( rv, v, 0x0000000F,  4 );
	__X( rv, v, 0x00000003,  2 );
	__X( rv, v, 0x00000001,  1 );
	#undef __X
	
	return rv;
}


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
	unsigned int *data;
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
	int ( *append )( struct __xlog_printer *printer, const char *text );
	int ( *optctl )( struct __xlog_printer *printer, int option, void *vptr, size_t size );
} xlog_printer_t;

typedef struct xlog_level_attr_tag {
	int format;
	const char *time_prefix;
	const char *time_suffix;
	const char *class_prefix;
	const char *class_suffix;
	const char *body_prefix;
	const char *body_suffix;
} xlog_level_attr_t;

typedef struct {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	int magic;
	#endif
	pthread_mutex_t lock;
	int options;
	char *savepath;
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
#define XLOG_PRINTER_TYPE_GET(options)	GET_BITS(options, 0, 4)


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
#define XLOG_MAGIC_PAYLOAD			XLOG_MAGIC_BUILD('X', 'P', 'L', 'D')
#endif


/** memory allocation for xlog */
#define XLOG_MALLOC(nbytes)			((nbytes) == 0 ? NULL : calloc(1, nbytes))
#define XLOG_REALLOC(ptr, nbytes)	realloc(ptr, nbytes)
#define XLOG_FREE(ptr)				free(ptr)
#define XLOG_STRDUP(str)			strdup(str)
#define XLOG_STRNDUP(str, n)		strndup(str, n)


/** Payload configurations */
// #define XLOG_PAYLOAD_ENABLE_DYNAMIC_BRIEF_INFO	/* uncomment to duplicate brief information at it's creation and free it when the payload be destroyed */

#define XLOG_PAYLOAD_MALLOC(size)			malloc(size)
#define XLOG_PAYLOAD_REALLOC(ptr, size)		realloc(ptr, size)
#define XLOG_PAYLOAD_FREE(ptr)				free(ptr)
#define XLOG_PAYLOAD_STRDUP(str)			strdup(str)
#define XLOG_PAYLOAD_TEXT_LENGTH(text)		strlen(text)
#define XLOG_PAYLOAD_NOREACHED()			assert(0)

/** Payload ID */
#define XLOG_PAYLOAD_ID_AUTO			0	/* Unspecified payload */
#define XLOG_PAYLOAD_ID_TEXT			1	/* Text payload */
#define XLOG_PAYLOAD_ID_BINARY			2	/* Binary payload */
#define XLOG_PAYLOAD_ID_LOG_TIME		3	/* Time payload */
#define XLOG_PAYLOAD_ID_LOG_CLASS		4	/* Log class payload(level(int) + layer-path(variable-length string)) */
#define XLOG_PAYLOAD_ID_LOG_POINT		5	/* Source location payload */
#define XLOG_PAYLOAD_ID_LOG_TASK		6	/* Task info payload, PID + TID and it's name */
#define XLOG_PAYLOAD_ID_LOG_BODY		7	/* Log body payload */

#define XLOG_PAYLOAD_ODYNAMIC			BIT_MASK(0) ///< dynamic allocating and need free beyond life span
#define XLOG_PAYLOAD_OFIXED 			BIT_MASK(1) ///< fixed size, alloc once or pre-allocated
#define XLOG_PAYLOAD_OALIGN 			BIT_MASK(2) ///< aligned buffer
#define XLOG_PAYLOAD_ORESERVING 		BIT_MASK(3) ///< reserving some bytes, it's useful for string or some protocol(s)
#define XLOG_PAYLOAD_OTEXT				BIT_MASK(4) ///< text payload
#define XLOG_PAYLOAD_OBINARY			BIT_MASK(5) ///< binary payload
#define XLOG_PAYLOAD_OALLOC_ONCE		XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OFIXED

typedef struct {
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	int magic;
	#endif
	const char  *brief;			///< payload brief
	unsigned int id;			///< payload identifier
	unsigned int
	options;		///< 0 ~ 7, mode; 8 ~ 11(2^0 ~ 2^15), alignment; 12 ~ 23(0 ~ 2^12 - 1), reserved bytes; 24 ~ 31, reserved for future use
	#define XLOG_PAYLOAD_BITS_MODE_LO	0
	#define XLOG_PAYLOAD_BITS_MODE_HI	8
	#define XLOG_PAYLOAD_BITS_ALIGN_LO	8
	#define XLOG_PAYLOAD_BITS_ALIGN_HI	12
	#define XLOG_PAYLOAD_BITS_RESVR_LO	12
	#define XLOG_PAYLOAD_BITS_RESVR_HI	24
	unsigned int offset;		///< offset of cursor
	unsigned int length;		///< length of data
	unsigned char data[0];		///< data field
} xlog_payload_t;

#define XLOG_PAYLOAD_TEST_OPTION(options, option)	(GET_BITS(options, XLOG_PAYLOAD_BITS_MODE_LO, XLOG_PAYLOAD_BITS_MODE_HI) & (option))

#define XLOG_PAYLOAD_GET_ALIGN_BITS(options)		GET_BITS(options, XLOG_PAYLOAD_BITS_ALIGN_LO, XLOG_PAYLOAD_BITS_ALIGN_HI)
#define XLOG_PAYLOAD_SET_ALIGN_BITS(options, bits)	SETV_BITS(options, XLOG_PAYLOAD_BITS_ALIGN_LO, XLOG_PAYLOAD_BITS_ALIGN_HI, bits)
#define XLOG_PAYLOAD_GET_RESERVED(options)			GET_BITS(options, XLOG_PAYLOAD_BITS_RESVR_LO, XLOG_PAYLOAD_BITS_RESVR_HI)
#define XLOG_PAYLOAD_SET_RESERVED(options, bytes)	SETV_BITS(options, XLOG_PAYLOAD_BITS_RESVR_LO, XLOG_PAYLOAD_BITS_RESVR_HI, bytes)

#define XLOG_PAYLOAD_RESIZEABLE(options)			(XLOG_PAYLOAD_TEST_OPTION(options, XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OFIXED) == XLOG_PAYLOAD_ODYNAMIC)
#define XLOG_PAYLOAD_TEXT_COMPATIBLE(options)		!XLOG_PAYLOAD_TEST_OPTION(options, XLOG_PAYLOAD_OBINARY)
#define XLOG_PAYLOAD_BINARY_COMPATIBLE(options)		!XLOG_PAYLOAD_TEST_OPTION(options, XLOG_PAYLOAD_OTEXT)

typedef struct xlog_time_tag {
	struct {
		long tv_sec;
		long tv_usec;
	} __attribute__( ( packed ) ) tv;
	struct {
		int tz_minuteswest;
		int tz_dsttime;
	} __attribute__( ( packed ) ) tz;
} __attribute__( ( packed ) ) xlog_time_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  create a payload object
 *
 * @param  id, payload identifier(@see XLOG_PAYLOAD_ID_xxx)
 *         brief, brief info of payload
 *         options, create options(@see XLOG_PAYLOAD_Oxxx)
 *         ..., variable arguments, correspond with options
 * @return pointer to `xlog_payload_t`, NULL if failed to allocate memory.
 *
 */
XLOG_PUBLIC( xlog_payload_t * ) xlog_payload_create( unsigned int id, const char *brief, int options, ... );

/**
 * @brief  resize data field of payload object
 *
 * @param  payload, payload object
 *         size, target size of payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_resize( xlog_payload_t **payload, size_t size );

/**
 * @brief  destory the payload object
 *
 * @param  payload, payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_destory( xlog_payload_t **payload );

/**
 * @brief  get pointer to data field of payload object
 *
 * @param  payload, payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( void * ) xlog_payload_data_vptr( const xlog_payload_t *payload );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         text, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_append_text( xlog_payload_t **payload, const char *text );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/args, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_append_text_va_list( xlog_payload_t **payload, const char *format, va_list args );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/..., text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_append_text_va( xlog_payload_t **payload, const char *format, ... );

/**
 * @brief  append binary to a payload object
 *
 * @param  payload, payload object
 *         vptr/size, binary data to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC( int ) xlog_payload_append_binary( xlog_payload_t **payload, const void *vptr, size_t size );

/**
 * @brief  print TEXT compatible payload
 *
 * @param  payload, payload object to print
 *         printer, printer to print the payload
 * @return length of printed payload data
 *
 */
XLOG_PUBLIC( int ) xlog_payload_print_TEXT(
	const xlog_payload_t *payload, xlog_printer_t *printer
);

/**
 * @brief  print BINARY compatible payload
 *
 * @param  payload, payload object to print
 *         printer, printer to print the payload
 * @return length of printed payload data
 *
 */
XLOG_PUBLIC( int ) xlog_payload_print_BINARY(
	const xlog_payload_t *payload, xlog_printer_t *printer
);

#ifdef __cplusplus
}
#endif

#endif
