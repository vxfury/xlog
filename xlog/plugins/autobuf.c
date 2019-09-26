#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include <xlog/plugins/autobuf.h>
#include <xlog/plugins/hexdump.h>

#include "internal.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
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
XLOG_PUBLIC( autobuf_t * ) autobuf_create( unsigned int id, const char *brief, int options, ... )
{
	autobuf_t *autobuf = NULL;
	int align = 0, reserved = 0;
	size_t length = 0;
	void *vptr = NULL;
	
	va_list ap;
	va_start( ap, options );
	if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ODYNAMIC ) ) {
		length = va_arg( ap, size_t );
		__XLOG_TRACE( "length = %zu", length );
		length += sizeof( autobuf_t );
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_OALIGN ) ) {
			align = va_arg( ap, int );
			XLOG_ASSERT( align > 0 && 0 == ( align & ( align - 1 ) ) );
			AUTOBUF_SET_ALIGN_BITS( options, BITS_FLS( align ) );
		}
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ORESERVING ) ) {
			reserved = va_arg( ap, int );
			__XLOG_TRACE( "reserved = %d", reserved );
			XLOG_ASSERT( reserved >= 0 );
			length += reserved;
			AUTOBUF_SET_RESERVED( options, reserved );
		}
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_OALIGN ) ) {
			length = XLOG_ALIGN_UP( length, align );
			__XLOG_TRACE( "length = %zu after alignment", length );
		}
		va_end( ap );
		
		autobuf = ( autobuf_t * )XLOG_MALLOC( length );
		if( NULL == autobuf ) {
			__XLOG_TRACE( "Out of memory" );
			return NULL;
		}
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		autobuf->magic = XLOG_MAGIC_PAYLOAD;
		#endif
		
		autobuf->id = id;
		#ifdef AUTOBUF_ENABLE_DYNAMIC_BRIEF_INFO
		autobuf->brief = AUTOBUF_STRDUP( brief );
		#else
		autobuf->brief = brief;
		#endif
		autobuf->options = options;
		autobuf->offset = 0;
		autobuf->length = length - sizeof( autobuf_t ) - reserved;
	} else {
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_OFIXED ) ) {
			options &= ~( AUTOBUF_OALIGN );
			vptr = va_arg( ap, void * );
			XLOG_ASSERT( vptr );
			length = va_arg( ap, size_t );
			__XLOG_TRACE( "Pre-Allocated: vptr = %p, size = %zu", vptr, length );
			if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ORESERVING ) ) {
				reserved = va_arg( ap, int );
				XLOG_ASSERT( reserved >= 0 );
				XLOG_ASSERT( length > reserved );
				AUTOBUF_SET_RESERVED( options, reserved );
			}
			va_end( ap );
			
			autobuf = ( autobuf_t * )XLOG_MALLOC( sizeof( autobuf_t ) + sizeof( void * ) );
			if( NULL == autobuf ) {
				return NULL;
			}
			#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
			autobuf->magic = XLOG_MAGIC_PAYLOAD;
			#endif
			
			autobuf->id = id;
			#ifdef AUTOBUF_ENABLE_DYNAMIC_BRIEF_INFO
			autobuf->brief = AUTOBUF_STRDUP( brief );
			#else
			autobuf->brief = brief;
			#endif
			autobuf->options = options;
			autobuf->offset = 0;
			autobuf->length = length - reserved;
			memcpy( autobuf->data, &vptr, sizeof( void * ) );
		} else {
			AUTOBUF_NOREACHED();
			/** NOREACHED */
		}
	}
	
	return autobuf;
}

/**
 * @brief  resize data field of autobuf object
 *
 * @param  autobuf, autobuf object
 *         size, target size of autobuf object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_resize( autobuf_t **autobuf, size_t size )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_RESIZEABLE( ( *autobuf )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	size_t reserved = AUTOBUF_GET_RESERVED( ( *autobuf )->options );
	size_t new_size = size + reserved + sizeof( autobuf_t );
	
	if( AUTOBUF_TEST_OPTION( ( *autobuf )->options, AUTOBUF_OALIGN ) ) {
		new_size = XLOG_ALIGN_UP( new_size, ( 0x1 << AUTOBUF_GET_ALIGN_BITS( ( *autobuf )->options ) ) );
	}
	
	autobuf_t *temp = ( autobuf_t * )XLOG_REALLOC( ( *autobuf ), new_size );
	if( temp == NULL ) {
		return ENOMEM;
	}
	*autobuf = temp;
	( *autobuf )->length = new_size - sizeof( autobuf_t ) - reserved;
	__XLOG_TRACE(
		"Payload: lenth = %u, offset = %u, reserved = %lu",
		( *autobuf )->length, ( *autobuf )->offset, AUTOBUF_GET_RESERVED( ( *autobuf )->options )
	);
	
	return 0;
}

/**
 * @brief  destory the autobuf object
 *
 * @param  autobuf, autobuf object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_destory( autobuf_t **autobuf )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	XLOG_FREE( *autobuf );
	*autobuf = NULL;
	
	return 0;
}

/**
 * @brief  get pointer to data field of autobuf object
 *
 * @param  autobuf, autobuf object
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( void * ) autobuf_data_vptr( const autobuf_t *autobuf )
{
	XLOG_ASSERT( autobuf );
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( autobuf->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error: maybe autobuf has been destoryed. magic is 0x%X.", autobuf->magic );
		return NULL;
	}
	#endif
	void *vptr = NULL;
	if( AUTOBUF_TEST_OPTION( autobuf->options, AUTOBUF_ODYNAMIC ) ) {
		vptr = ( void * )autobuf->data;
	} else {
		memcpy( &vptr, autobuf->data, sizeof( void * ) );
	}
	
	return vptr;
}

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         text, text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_append_text( autobuf_t **autobuf, const char *text )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
		|| text == NULL
	) {
		__XLOG_TRACE( "Invalid parameters. text = %p", text );
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	size_t textlen = AUTOBUF_TEXT_LENGTH( text );
	if( ( *autobuf )->offset + textlen < ( *autobuf )->length ) {
		char *ptr = ( char * )autobuf_data_vptr( *autobuf );
		memcpy( ptr + ( *autobuf )->offset, text, textlen );
		( *autobuf )->offset += textlen;
		*( ptr + ( *autobuf )->offset ) = '\0';
		
		return 0;
	} else if( AUTOBUF_RESIZEABLE( ( *autobuf )->options ) ) {
		int rv = autobuf_resize( autobuf, ( *autobuf )->offset + textlen + 1 );
		__XLOG_TRACE( "Resize triggered, error = %d.", rv );
		if( 0 == rv ) {
			char *ptr = ( char * )autobuf_data_vptr( *autobuf );

			memcpy( ptr + ( *autobuf )->offset, text, textlen );
			( *autobuf )->offset += textlen;
			*( ptr + ( *autobuf )->offset ) = '\0';
		}
		
		return rv;
	}
	
	return 0;
}

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         format/ap, text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_append_text_va_list( autobuf_t **autobuf, const char *format, va_list ap )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
		|| format == NULL
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	va_list ap_bkp;
	va_copy( ap_bkp, ap );
	
	if( ( *autobuf )->offset < ( *autobuf )->length ) {
		char *ptr = ( char * )autobuf_data_vptr( *autobuf );
		
		int len = vsnprintf( ptr + ( *autobuf )->offset, ( *autobuf )->length - ( *autobuf )->offset, format, ap );
		if( len < 0 ) {
			return EIO;
		} else if( len < ( *autobuf )->length - ( *autobuf )->offset ) {
			( *autobuf )->offset += len;
			return 0;
		} else if( AUTOBUF_RESIZEABLE( ( *autobuf )->options ) ) {
			int rv = autobuf_resize( autobuf, ( *autobuf )->offset + len + 1 );
			__XLOG_TRACE( "Resize triggered, error = %d.", rv );
			if( 0 == rv ) {
				ptr = ( char * )autobuf_data_vptr( *autobuf );
				len = vsnprintf( ptr + ( *autobuf )->offset, ( *autobuf )->length - ( *autobuf )->offset, format, ap_bkp );
				va_end( ap_bkp );
				( *autobuf )->offset += len;
			}
			
			return rv;
		} else {
			( *autobuf )->offset = ( *autobuf )->length;
			return EOVERFLOW;
		}
	} else {
		return EOVERFLOW;
	}
	
	return EINVAL;
}

/**
 * @brief  append text to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         format/..., text to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_append_text_va( autobuf_t **autobuf, const char *format, ... )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		return EINVAL;
	}
	#endif
	
	va_list ap;
	va_start( ap, format );
	int rv = autobuf_append_text_va_list( autobuf, format, ap );
	va_end( ap );
	
	return rv;
}

/**
 * @brief  append binary to a autobuf object
 *
 * @param  autobuf, autobuf object
 *         vptr/size, binary data to append
 * @return error code(@see Exxx).
 *
 */
XLOG_PUBLIC( int ) autobuf_append_binary( autobuf_t **autobuf, const void *vptr, size_t size )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_BINARY_COMPATIBLE( ( *autobuf )->options )
	) {
		__XLOG_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( ( *autobuf )->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error: maybe autobuf given has been destoryed." );
		return EINVAL;
	}
	#endif
	
	if( ( *autobuf )->offset + size <= ( *autobuf )->length ) {
		char *ptr = ( char * )autobuf_data_vptr( *autobuf );
		memcpy( ptr + ( *autobuf )->offset, vptr, size );
		( *autobuf )->offset += size;
		
		return 0;
	} else if( AUTOBUF_RESIZEABLE( ( *autobuf )->options ) ) {
		int rv = autobuf_resize( autobuf, ( *autobuf )->offset + size );
		__XLOG_TRACE( "Resize triggered, error = %d.", rv );
		if( 0 == rv ) {
			char *ptr = ( char * )autobuf_data_vptr( *autobuf );

			memcpy( ptr + ( *autobuf )->offset, vptr, size );
			( *autobuf )->offset += size;
		}
		__XLOG_TRACE( "test" );
		
		return rv;
	} else {
		__XLOG_TRACE( "Overflow, terminate appending." );
	}
	
	return EINVAL;
}

/**
 * @brief  print TEXT compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) autobuf_print_TEXT(
	const autobuf_t *autobuf, xlog_printer_t *printer
)
{
	XLOG_ASSERT( autobuf );
	XLOG_ASSERT( AUTOBUF_TEXT_COMPATIBLE( autobuf->options ) );
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( autobuf->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error, magic is NOT XLOG_MAGIC_PAYLOAD." );
		return 0;
	}
	#endif
	
	return printer->append( printer, ( const char * )autobuf_data_vptr( autobuf ) );
}

static void hexdump_printline( uintmax_t cursor, const char *dumpline, void *arg )
{
	xlog_printer_t *printer = ( xlog_printer_t * )arg;
	
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
 * @brief  print BINARY compatible autobuf
 *
 * @param  autobuf, autobuf object to print
 *         printer, printer to print the autobuf
 * @return length of printed autobuf data
 *
 */
XLOG_PUBLIC( int ) autobuf_print_BINARY(
	const autobuf_t *autobuf, xlog_printer_t *printer
)
{
	XLOG_ASSERT( autobuf );
	XLOG_ASSERT( printer );
	
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( autobuf->magic != XLOG_MAGIC_PAYLOAD ) {
		__XLOG_TRACE( "Runtime error: maybe autobuf has been destoryed." );
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
	options.end = options.start + autobuf->offset;
	if( printer->optctl ) {
		printer->optctl( printer, XLOG_PRINTER_CTRL_LOCK, NULL, 0 );
	}
	int length = __hexdump( autobuf_data_vptr( autobuf ), &options, hexdump_memory_readline, hexdump_printline, printer );
	if( printer->optctl ) {
		printer->optctl( printer, XLOG_PRINTER_CTRL_UNLOCK, NULL, 0 );
	}
	
	return length;
}
