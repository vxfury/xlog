#ifndef __XLOG_PAYLOAD_H
#define __XLOG_PAYLOAD_H

#include <xlog/xlog_config.h>
#include <xlog/xlog_helper.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

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

enum {
	XLOG_PAYLOAD_ODYNAMIC 	= 0x1 << 0, 	///< dynamic allocating and need free beyond life span
	XLOG_PAYLOAD_OFIXED 	= 0x1 << 1, 	///< fixed size, alloc once or pre-allocated
	XLOG_PAYLOAD_OALIGN 	= 0x1 << 2, 	///< aligned buffer
	XLOG_PAYLOAD_ORESERVING = 0x1 << 3, 	///< reserving some bytes, it's useful for string or some protocol(s)
	XLOG_PAYLOAD_OTEXT		= 0x1 << 4,		///< text payload
	XLOG_PAYLOAD_OBINARY	= 0x1 << 5,		///< binary payload
};
#define XLOG_PAYLOAD_OALLOC_ONCE		XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OFIXED

typedef struct {
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

#define XLOG_PAYLOAD_MIN(a, b)						((a) <= (b) ? (a) : (b))
#define XLOG_PAYLOAD_MAX(a, b)						((a) >= (b) ? (a) : (b))

#define XLOG_PAYLOAD_ALIGN_DOWN(a, size)			(a & (~(size-1)))
#define XLOG_PAYLOAD_ALIGN_UP(a, size)				((a+size-1) & (~(size-1)))

#define XLOG_PAYLOAD_QSQRT(v)						(0x1 << ((__fls(v) + 1) >> 1))
#define XLOG_PAYLOAD_GRWSIZE(old, req)				(XLOG_PAYLOAD_QSQRT(old) + (req))
#define XLOG_PAYLOAD_DECSIZE(old, len)				XLOG_PAYLOAD_MAX(0, (len) - ((old) + XLOG_PAYLOAD_QSQRT(old)))

#define XLOG_PAYLOAD_TRACE(...)						// printf("[TRACE] (%s:%d) ", __func__, __LINE__), printf( __VA_ARGS__ ), printf( "\n" )

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
XLOG_PUBLIC(xlog_payload_t *) xlog_payload_create( unsigned int id, const char *brief, int options, ... );

/**
 * @brief  resize data field of payload object
 *
 * @param  payload, payload object
 *         size, target size of payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_resize( xlog_payload_t **payload, size_t size );

/**
 * @brief  destory the payload object
 *
 * @param  payload, payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_destory( xlog_payload_t **payload );

/**
 * @brief  get pointer to data field of payload object
 *
 * @param  payload, payload object
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(void *) xlog_payload_data_vptr( const xlog_payload_t *payload );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         text, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text( xlog_payload_t **payload, const char *text );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/args, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text_va_list( xlog_payload_t **payload, const char *format, va_list args );

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/..., text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text_va( xlog_payload_t **payload, const char *format, ... );

/**
 * @brief  append binary to a payload object
 *
 * @param  payload, payload object
 *         vptr/size, binary data to append
 * @return error code(@see XLOG_Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_binary( xlog_payload_t **payload, const void *vptr, size_t size );

/**
 * @brief  print TEXT compatible payload
 *
 * @param  payload, payload object to print
 *         printer, printer to print the payload
 * @return length of printed payload data
 *
 */
XLOG_PUBLIC(int) xlog_payload_print_TEXT(
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
XLOG_PUBLIC(int) xlog_payload_print_BINARY(
    const xlog_payload_t *payload, xlog_printer_t *printer
);

#ifdef __cplusplus
}
#endif

#endif
