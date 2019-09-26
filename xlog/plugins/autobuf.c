#include <xlog/plugins/autobuf.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

#define AUTOBUF_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define AUTOBUF_ALIGN_UP(size, align) ((align+size-1) & (~(align-1)))
#define AUTOBUF_TRACE(...)

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
API_PUBLIC( autobuf_t * ) autobuf_create( unsigned int id, const char *brief, int options, ... )
{
	autobuf_t *autobuf = NULL;
	int align = 0, reserved = 0;
	size_t length = 0;
	void *vptr = NULL;
	
	va_list ap;
	va_start( ap, options );
	if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ODYNAMIC ) ) {
		length = va_arg( ap, size_t );
		AUTOBUF_TRACE( "length = %zu", length );
		length += sizeof( autobuf_t );
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_OALIGN ) ) {
			align = va_arg( ap, int );
			AUTOBUF_ASSERT( align > 0 && 0 == ( align & ( align - 1 ) ) );
			AUTOBUF_SET_ALIGN_BITS( options, BITS_FLS( align ) );
		}
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ORESERVING ) ) {
			reserved = va_arg( ap, int );
			AUTOBUF_TRACE( "reserved = %d", reserved );
			AUTOBUF_ASSERT( reserved >= 0 );
			length += reserved;
			AUTOBUF_SET_RESERVED( options, reserved );
		}
		if( AUTOBUF_TEST_OPTION( options, AUTOBUF_OALIGN ) ) {
			length = AUTOBUF_ALIGN_UP( length, align );
			AUTOBUF_TRACE( "length = %zu after alignment", length );
		}
		va_end( ap );
		
		autobuf = ( autobuf_t * )AUTOBUF_MALLOC( length );
		if( NULL == autobuf ) {
			AUTOBUF_TRACE( "Out of memory" );
			return NULL;
		}
		
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
			AUTOBUF_ASSERT( vptr );
			length = va_arg( ap, size_t );
			AUTOBUF_TRACE( "Pre-Allocated: vptr = %p, size = %zu", vptr, length );
			if( AUTOBUF_TEST_OPTION( options, AUTOBUF_ORESERVING ) ) {
				reserved = va_arg( ap, int );
				AUTOBUF_ASSERT( reserved >= 0 );
				AUTOBUF_ASSERT( length > reserved );
				AUTOBUF_SET_RESERVED( options, reserved );
			}
			va_end( ap );
			
			autobuf = ( autobuf_t * )AUTOBUF_MALLOC( sizeof( autobuf_t ) + sizeof( void * ) );
			if( NULL == autobuf ) {
				return NULL;
			}
			
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
API_PUBLIC( int ) autobuf_resize( autobuf_t **autobuf, size_t size )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_RESIZEABLE( ( *autobuf )->options )
	) {
		AUTOBUF_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	size_t reserved = AUTOBUF_GET_RESERVED( ( *autobuf )->options );
	size_t new_size = size + reserved + sizeof( autobuf_t );
	
	if( AUTOBUF_TEST_OPTION( ( *autobuf )->options, AUTOBUF_OALIGN ) ) {
		new_size = AUTOBUF_ALIGN_UP( new_size, ( 0x1 << AUTOBUF_GET_ALIGN_BITS( ( *autobuf )->options ) ) );
	}
	
	autobuf_t *temp = ( autobuf_t * )AUTOBUF_REALLOC( ( *autobuf ), new_size );
	if( temp == NULL ) {
		return ENOMEM;
	}
	*autobuf = temp;
	( *autobuf )->length = new_size - sizeof( autobuf_t ) - reserved;
	AUTOBUF_TRACE(
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
API_PUBLIC( int ) autobuf_destory( autobuf_t **autobuf )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
	) {
		AUTOBUF_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	AUTOBUF_FREE( *autobuf );
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
API_PUBLIC( void * ) autobuf_data_vptr( const autobuf_t *autobuf )
{
	AUTOBUF_ASSERT( autobuf );
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
API_PUBLIC( int ) autobuf_append_text( autobuf_t **autobuf, const char *text )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
		|| text == NULL
	) {
		AUTOBUF_TRACE( "Invalid parameters. text = %p", text );
		return EINVAL;
	}
	
	size_t textlen = AUTOBUF_TEXT_LENGTH( text );
	if( ( *autobuf )->offset + textlen < ( *autobuf )->length ) {
		char *ptr = ( char * )autobuf_data_vptr( *autobuf );
		memcpy( ptr + ( *autobuf )->offset, text, textlen );
		( *autobuf )->offset += textlen;
		*( ptr + ( *autobuf )->offset ) = '\0';
		
		return 0;
	} else if( AUTOBUF_RESIZEABLE( ( *autobuf )->options ) ) {
		int rv = autobuf_resize( autobuf, ( *autobuf )->offset + textlen + 1 );
		AUTOBUF_TRACE( "Resize triggered, error = %d.", rv );
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
API_PUBLIC( int ) autobuf_append_text_va_list( autobuf_t **autobuf, const char *format, va_list ap )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
		|| format == NULL
	) {
		AUTOBUF_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
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
			AUTOBUF_TRACE( "Resize triggered, error = %d.", rv );
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
API_PUBLIC( int ) autobuf_append_text_va( autobuf_t **autobuf, const char *format, ... )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_TEXT_COMPATIBLE( ( *autobuf )->options )
	) {
		AUTOBUF_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
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
API_PUBLIC( int ) autobuf_append_binary( autobuf_t **autobuf, const void *vptr, size_t size )
{
	if(
		autobuf == NULL
		|| *autobuf == NULL
		|| !AUTOBUF_BINARY_COMPATIBLE( ( *autobuf )->options )
	) {
		AUTOBUF_TRACE( "Invalid parameters." );
		return EINVAL;
	}
	
	if( ( *autobuf )->offset + size <= ( *autobuf )->length ) {
		char *ptr = ( char * )autobuf_data_vptr( *autobuf );
		memcpy( ptr + ( *autobuf )->offset, vptr, size );
		( *autobuf )->offset += size;
		
		return 0;
	} else if( AUTOBUF_RESIZEABLE( ( *autobuf )->options ) ) {
		int rv = autobuf_resize( autobuf, ( *autobuf )->offset + size );
		AUTOBUF_TRACE( "Resize triggered, error = %d.", rv );
		if( 0 == rv ) {
			char *ptr = ( char * )autobuf_data_vptr( *autobuf );

			memcpy( ptr + ( *autobuf )->offset, vptr, size );
			( *autobuf )->offset += size;
		}
		
		return rv;
	} else {
		AUTOBUF_TRACE( "Overflow, terminate appending." );
	}
	
	return EINVAL;
}
