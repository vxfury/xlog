#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

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
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
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
XLOG_PUBLIC(xlog_payload_t *) xlog_payload_create( unsigned int id, const char *brief, int options, ... )
{
	xlog_payload_t *payload = NULL;
	int align = 0, reserved = 0;
	size_t length = 0;
	void *vptr = NULL;
	
	va_list ap;
	va_start( ap, options );
	if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_ODYNAMIC ) ) {
		length = va_arg( ap, size_t );
		__XLOG_TRACE( "length = %zu", length );
		length += sizeof( xlog_payload_t );
		if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_OALIGN ) ) {
			align = va_arg( ap, int );
			XLOG_ASSERT( align > 0 && 0 == ( align & ( align - 1 ) ) );
			XLOG_PAYLOAD_SET_ALIGN_BITS( options, BITS_FLS( align ) );
		}
		if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_ORESERVING ) ) {
			reserved = va_arg( ap, int );
			__XLOG_TRACE( "reserved = %d", reserved );
			XLOG_ASSERT( reserved >= 0 );
			length += reserved;
			XLOG_PAYLOAD_SET_RESERVED( options, reserved );
		}
		if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_OALIGN ) ) {
			length = XLOG_ALIGN_UP( length, align );
			__XLOG_TRACE( "length = %zu after alignment", length );
		}
		va_end( ap );
		
		payload = ( xlog_payload_t * )XLOG_MALLOC( length );
		if( NULL == payload ) {
			__XLOG_TRACE( "Out of memory" );
			return NULL;
		}
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		payload->magic = XLOG_MAGIC_PAYLOAD;
		#endif
		
		payload->id = id;
		#ifdef XLOG_PAYLOAD_ENABLE_DYNAMIC_BRIEF_INFO
		payload->brief = XLOG_PAYLOAD_STRDUP( brief );
		#else
		payload->brief = brief;
		#endif
		payload->options = options;
		payload->offset = 0;
		payload->length = length - sizeof( xlog_payload_t ) - reserved;
	} else {
		if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_OFIXED ) ) {
			options &= ~( XLOG_PAYLOAD_OALIGN );
			vptr = va_arg( ap, void * );
			XLOG_ASSERT( vptr );
			length = va_arg( ap, size_t );
			__XLOG_TRACE( "Pre-Allocated: vptr = %p, size = %zu", vptr, length );
			if( XLOG_PAYLOAD_TEST_OPTION( options, XLOG_PAYLOAD_ORESERVING ) ) {
				reserved = va_arg( ap, int );
				XLOG_ASSERT( reserved >= 0 );
				XLOG_ASSERT( length > reserved );
				XLOG_PAYLOAD_SET_RESERVED( options, reserved );
			}
			va_end( ap );
			
			payload = ( xlog_payload_t * )XLOG_MALLOC( sizeof( xlog_payload_t ) + sizeof( void * ) );
			if( NULL == payload ) {
				return NULL;
			}
			
			payload->id = id;
			#ifdef XLOG_PAYLOAD_ENABLE_DYNAMIC_BRIEF_INFO
			payload->brief = XLOG_PAYLOAD_STRDUP( brief );
			#else
			payload->brief = brief;
			#endif
			payload->options = options;
			payload->offset = 0;
			payload->length = length - reserved;
			memcpy( payload->data, &vptr, sizeof( void * ) );
		} else {
			XLOG_PAYLOAD_NOREACHED();
			/** NOREACHED */
		}
	}
	
	return payload;
}

/**
 * @brief  resize data field of payload object
 *
 * @param  payload, payload object
 *         size, target size of payload object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_resize( xlog_payload_t **payload, size_t size )
{
	if(
	    payload == NULL
	    || *payload == NULL
	    || !XLOG_PAYLOAD_RESIZEABLE( ( *payload )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	size_t reserved = XLOG_PAYLOAD_GET_RESERVED( ( *payload )->options );
	size_t new_size = size + reserved + sizeof( xlog_payload_t );
	
	if( XLOG_PAYLOAD_TEST_OPTION( ( *payload )->options, XLOG_PAYLOAD_OALIGN ) ) {
		new_size = XLOG_ALIGN_UP( new_size, ( 0x1 << XLOG_PAYLOAD_GET_ALIGN_BITS( ( *payload )->options ) ) );
	}
	
	xlog_payload_t *temp = (xlog_payload_t *)XLOG_REALLOC( ( *payload ), new_size );
	if( temp == NULL ) {
		return ENOMEM;
	}
	*payload = temp;
	( *payload )->length = new_size - sizeof( xlog_payload_t ) - reserved;
	__XLOG_TRACE(
		"Payload: lenth = %u, offset = %u, reserved = %lu",
		(*payload)->length, (*payload)->offset, XLOG_PAYLOAD_GET_RESERVED( (*payload)->options )
	);
	
	return 0;
}

/**
 * @brief  destory the payload object
 *
 * @param  payload, payload object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_destory( xlog_payload_t **payload )
{
	if(
	    payload == NULL
	    || *payload == NULL
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	XLOG_FREE( *payload );
	*payload = NULL;
	
	return 0;
}

/**
 * @brief  get pointer to data field of payload object
 *
 * @param  payload, payload object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(void *) xlog_payload_data_vptr( const xlog_payload_t *payload )
{
	XLOG_ASSERT( payload );
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( payload->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error: maybe payload has been destoryed. magic is 0x%X.", payload->magic );
		return NULL;
	}
	#endif
	void *vptr = NULL;
	if( XLOG_PAYLOAD_TEST_OPTION( payload->options, XLOG_PAYLOAD_ODYNAMIC ) ) {
		vptr = ( void * )payload->data;
	} else {
		memcpy( &vptr, payload->data, sizeof( void * ) );
	}
	
	return vptr;
}

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         text, text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text( xlog_payload_t **payload, const char *text )
{
	if(
	    payload == NULL
	    || *payload == NULL
	    || !XLOG_PAYLOAD_TEXT_COMPATIBLE( ( *payload )->options )
	    || text == NULL
	) {
		__XLOG_TRACE( "Invalid parameters. text = %p", text );
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	size_t textlen = XLOG_PAYLOAD_TEXT_LENGTH( text );
	if( ( *payload )->offset + textlen < ( *payload )->length ) {
		char *ptr = ( char * )xlog_payload_data_vptr( *payload );
		__XLOG_TRACE( "ptr = %p", ptr );
		memcpy( ptr + ( *payload )->offset, text, textlen );
		( *payload )->offset += textlen;
		*( ptr + ( *payload )->offset ) = '\0';
		
		return 0;
	} else if( XLOG_PAYLOAD_RESIZEABLE( ( *payload )->options ) ) {
		int rv = xlog_payload_resize( payload, ( *payload )->offset + textlen + 1 );
		__XLOG_TRACE( "Resize triggered, error = %d.", rv );
		if( 0 == rv ) {
			char *ptr = ( char * )xlog_payload_data_vptr( *payload );
			__XLOG_TRACE( "ptr = %p", ptr );
			memcpy( ptr + ( *payload )->offset, text, textlen );
			( *payload )->offset += textlen;
			*( ptr + ( *payload )->offset ) = '\0';
		}
		
		return rv;
	}
	
	return 0;
}

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/ap, text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text_va_list( xlog_payload_t **payload, const char *format, va_list ap )
{
	if(
	    payload == NULL
	    || *payload == NULL
	    || !XLOG_PAYLOAD_TEXT_COMPATIBLE( ( *payload )->options )
	    || format == NULL
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	va_list ap_bkp;
	va_copy( ap_bkp, ap );
	
	if( ( *payload )->offset < ( *payload )->length ) {
		char *ptr = ( char * )xlog_payload_data_vptr( *payload );
		__XLOG_TRACE( "ptr = %p", ptr );
		
		int len = vsnprintf( ptr + ( *payload )->offset, ( *payload )->length - ( *payload )->offset, format, ap );
		if( len < 0 ) {
			return EIO;
		} else if( len < ( *payload )->length - ( *payload )->offset ) {
			( *payload )->offset += len;
			return 0;
		} else if( XLOG_PAYLOAD_RESIZEABLE( ( *payload )->options ) ) {
			int rv = xlog_payload_resize( payload, ( *payload )->offset + len + 1 );
			__XLOG_TRACE( "Resize triggered, error = %d.", rv );
			if( 0 == rv ) {
				ptr = ( char * )xlog_payload_data_vptr( *payload );
				__XLOG_TRACE( "ptr = %p", ptr );
				len = vsnprintf( ptr + ( *payload )->offset, ( *payload )->length - ( *payload )->offset, format, ap_bkp );
				va_end( ap_bkp );
				( *payload )->offset += len;
			}
			
			return rv;
		} else {
			( *payload )->offset = ( *payload )->length;
			return EOVERFLOW;
		}
	} else {
		return EOVERFLOW;
	}
	
	return EINVAL;
}

/**
 * @brief  append text to a payload object
 *
 * @param  payload, payload object
 *         format/..., text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_text_va( xlog_payload_t **payload, const char *format, ... )
{
	if(
	    payload == NULL
	    || *payload == NULL
	    || !XLOG_PAYLOAD_TEXT_COMPATIBLE( ( *payload )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif

	va_list ap;
	va_start( ap, format );
	int rv = xlog_payload_append_text_va_list( payload, format, ap );
	va_end( ap );

	return rv;
}

/**
 * @brief  append binary to a payload object
 *
 * @param  payload, payload object
 *         vptr/size, binary data to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC(int) xlog_payload_append_binary( xlog_payload_t **payload, const void *vptr, size_t size )
{
	if(
	    payload == NULL
	    || *payload == NULL
	    || !XLOG_PAYLOAD_BINARY_COMPATIBLE( ( *payload )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *payload )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	if( ( *payload )->offset + size <= ( *payload )->length ) {
		char *ptr = ( char * )xlog_payload_data_vptr( *payload );
		__XLOG_TRACE( "ptr = %p", ptr );
		memcpy( ptr + ( *payload )->offset, vptr, size );
		( *payload )->offset += size;
		
		return 0;
	} else if( XLOG_PAYLOAD_RESIZEABLE( ( *payload )->options ) ) {
		int rv = xlog_payload_resize( payload, ( *payload )->offset + size );
		__XLOG_TRACE( "Resize triggered, error = %d.", rv );
		if( 0 == rv ) {
			char *ptr = ( char * )xlog_payload_data_vptr( *payload );
			__XLOG_TRACE( "ptr = %p", ptr );
			memcpy( ptr + ( *payload )->offset, vptr, size );
			( *payload )->offset += size;
		}
		__XLOG_TRACE( "test" );
		
		return rv;
	}
	
	return EINVAL;
}

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
)
{
	XLOG_ASSERT( payload );
	XLOG_ASSERT( XLOG_PAYLOAD_TEXT_COMPATIBLE( payload->options ) );
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( payload->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error, magic is NOT XLOG_MAGIC_PAYLOAD." );
		return 0;
	}
	#endif
	
	return printer->append( printer, (const char *)xlog_payload_data_vptr( payload ) );
}

static void hexdump_printline( uintmax_t cursor, const char *dumpline, void *arg )
{
	xlog_printer_t *printer = (xlog_printer_t *)arg;

	char buffer[128];
	snprintf(
		buffer, sizeof( buffer ),
		"%5jx%03jx  %s\n", cursor >> 12, cursor & 0xFFF, dumpline
	);
	printer->append( printer, buffer );
}

static int hexdump_memory_readline( const void *addr, off_t offset, void *buffer, size_t size )
{
	memcpy( buffer, ( char * )addr + offset, size );
	
	return size;
}

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
)
{
	XLOG_ASSERT( payload );
	XLOG_ASSERT( printer );
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( payload->magic != XLOG_MAGIC_PAYLOAD ) {
		return 0;
	}
	#endif

	hexdump_options_t options = {
		.start = 0,
		.end = -1,
		.columns = 16,
		.groupsize = 8,
		.use_formatting = false,
	};
	options.end = options.start + payload->offset;
	printer->control( printer, XLOG_PRINTER_CTRL_LOCK, NULL );
	int length = __hexdump( payload->data, &options, hexdump_memory_readline, hexdump_printline, printer );
	printer->control( printer, XLOG_PRINTER_CTRL_UNLOCK, NULL );
	
	return length;
}
