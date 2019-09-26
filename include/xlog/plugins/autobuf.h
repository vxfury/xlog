#ifndef __AUTOBUF_H
#define __AUTOBUF_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#include <xlog/plugins/bitops.h>

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

#ifndef API_PUBLIC
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(XLOG_API_VISIBILITY)
#define API_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define API_PUBLIC(type) type
#endif
#endif

/** Autobuf configurations */
// #define AUTOBUF_ENABLE_DYNAMIC_BRIEF_INFO	/* uncomment to duplicate brief information at it's creation and free it when the autobuf be destroyed */

#define AUTOBUF_MALLOC(size)		malloc(size)
#define AUTOBUF_REALLOC(ptr, size)	realloc(ptr, size)
#define AUTOBUF_FREE(ptr)			free(ptr)
#define AUTOBUF_STRDUP(str)			strdup(str)
#define AUTOBUF_TEXT_LENGTH(text)	strlen(text)
#define AUTOBUF_ASSERT(cond)		assert(cond)
#define AUTOBUF_NOREACHED()			assert(0)

#define AUTOBUF_TRACE(...) // fprintf( stderr, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> " ), fprintf( stderr, __VA_ARGS__ ), fprintf( stderr, "\r\n" )

#define AUTOBUF_ODYNAMIC			BIT_MASK(0) ///< dynamic allocating and need free beyond life span
#define AUTOBUF_OFIXED 				BIT_MASK(1) ///< fixed size, alloc once or pre-allocated
#define AUTOBUF_OALIGN 				BIT_MASK(2) ///< aligned buffer
#define AUTOBUF_ORESERVING 			BIT_MASK(3) ///< reserving some bytes, it's useful for string or some protocol(s)
#define AUTOBUF_OTEXT				BIT_MASK(4) ///< text autobuf
#define AUTOBUF_OBINARY				BIT_MASK(5) ///< binary autobuf
#define AUTOBUF_OALLOC_ONCE			AUTOBUF_ODYNAMIC | AUTOBUF_OFIXED

typedef struct {
	const char  *brief;			///< autobuf brief
	unsigned int id;			///< autobuf identifier
	unsigned int options;		///< 0 ~ 7, mode; 8 ~ 11(2^0 ~ 2^15), alignment; 12 ~ 23(0 ~ 2^12 - 1), reserved bytes; 24 ~ 31, reserved for future use
	#define AUTOBUF_BITS_MODE_LO	0
	#define AUTOBUF_BITS_MODE_HI	8
	#define AUTOBUF_BITS_ALIGN_LO	8
	#define AUTOBUF_BITS_ALIGN_HI	12
	#define AUTOBUF_BITS_RESVR_LO	12
	#define AUTOBUF_BITS_RESVR_HI	24
	unsigned int offset;		///< offset of cursor
	unsigned int length;		///< length of data
	unsigned char data[0];		///< data field
} autobuf_t;

#define AUTOBUF_TEST_OPTION(options, option)	(GET_BITS(options, AUTOBUF_BITS_MODE_LO, AUTOBUF_BITS_MODE_HI) & (option))

#define AUTOBUF_GET_ALIGN_BITS(options)		GET_BITS(options, AUTOBUF_BITS_ALIGN_LO, AUTOBUF_BITS_ALIGN_HI)
#define AUTOBUF_SET_ALIGN_BITS(options, bits)	SETV_BITS(options, AUTOBUF_BITS_ALIGN_LO, AUTOBUF_BITS_ALIGN_HI, bits)
#define AUTOBUF_GET_RESERVED(options)			GET_BITS(options, AUTOBUF_BITS_RESVR_LO, AUTOBUF_BITS_RESVR_HI)
#define AUTOBUF_SET_RESERVED(options, bytes)	SETV_BITS(options, AUTOBUF_BITS_RESVR_LO, AUTOBUF_BITS_RESVR_HI, bytes)

#define AUTOBUF_RESIZEABLE(options)			(AUTOBUF_TEST_OPTION(options, AUTOBUF_ODYNAMIC | AUTOBUF_OFIXED) == AUTOBUF_ODYNAMIC)
#define AUTOBUF_TEXT_COMPATIBLE(options)		!AUTOBUF_TEST_OPTION(options, AUTOBUF_OBINARY)
#define AUTOBUF_BINARY_COMPATIBLE(options)		!AUTOBUF_TEST_OPTION(options, AUTOBUF_OTEXT)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  create a autobuf object
 *
 * @param  id, autobuf identifier(@see PAYLOAD_ID_xxx)
 *         brief, brief info of autobuf
 *         options, create options(@see AUTOBUF_Oxxx)
 *         ..., variable arguments, correspond with options
 * @return pointer to `autobuf_t`, NULL if failed to allocate memory.
 *
 */
API_PUBLIC( autobuf_t * ) autobuf_create( unsigned int id, const char *brief, int options, ... );

/**
 * @brief  resize data field of autobuf object
 *
 * @param  autobuf, autobuf object
 *         size, target size of autobuf object
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_resize( autobuf_t **autobuf, size_t size );

/**
 * @brief  destory the autobuf object
 *
 * @param  autobuf, autobuf object
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_destory( autobuf_t **autobuf );

/**
 * @brief  get pointer to data field of autobuf object
 *
 * @param  autobuf, autobuf object
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( void * ) autobuf_data_vptr( const autobuf_t *autobuf );

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         text, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_append_text( autobuf_t **autobuf, const char *text );

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         format/args, text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_append_text_va_list( autobuf_t **autobuf, const char *format, va_list args );

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         format/..., text to append
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_append_text_va( autobuf_t **autobuf, const char *format, ... );

/**
 * @brief  append binary to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         vptr/size, binary data to append
 * @return error code(@see XLOG_Exxx).
 *
 */
API_PUBLIC( int ) autobuf_append_binary( autobuf_t **autobuf, const void *vptr, size_t size );

#ifdef __cplusplus
}
#endif

#endif
