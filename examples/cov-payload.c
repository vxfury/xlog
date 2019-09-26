#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static unsigned long random_integer( void )
{
	static bool initialized = false;
	if( !initialized ) {
		unsigned long seed;
		
		int fd = open( "/dev/urandom", O_RDONLY );
		if( fd < 0 || read( fd, &seed, sizeof( seed ) ) < 0 ) {
			seed = time( NULL );
		}
		if( fd >= 0 ) {
			close( fd );
			fd = -1;
		}
		
		srand( ( unsigned int )seed );
		initialized = true;
	}
	
	return rand();
}

int main( int argc, char **argv )
{
	( void )argc;
	( void )argv;
	
	{
		xlog_payload_t *payload = xlog_payload_create(
		    XLOG_PAYLOAD_ID_TEXT, "Text",
		    XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN | XLOG_PAYLOAD_OTEXT,
		    1024, 32
		);
		
		char buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer() % 26 + 'a';
		}
		buff[sizeof( buff ) - 1] = '\0';
		
		xlog_payload_append_text( &payload, buff );
		xlog_payload_print_TEXT( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
		xlog_payload_destory( &payload );
		
		log_r( "buffer = %s\n", buff );
	}
	
	{
		xlog_payload_t *payload = xlog_payload_create(
		    XLOG_PAYLOAD_ID_BINARY, "Binary",
		    XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN | XLOG_PAYLOAD_OBINARY,
		    1024, 32
		);
		
		unsigned long buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer();
		}
		
		xlog_payload_append_binary( &payload, buff, sizeof( buff ) );
		xlog_payload_print_BINARY( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
		xlog_payload_destory( &payload );
	}
	
	{
		char buffer[1024];
		xlog_payload_t *payload = xlog_payload_create(
		    XLOG_PAYLOAD_ID_BINARY, "Binary",
		    XLOG_PAYLOAD_OFIXED | XLOG_PAYLOAD_OBINARY,
		    buffer, sizeof( buffer )
		);
		
		if( payload ) {
			unsigned long buff[128];
			for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
				buff[i] = random_integer();
			}
			
			xlog_payload_append_binary( &payload, buff, sizeof( buff ) );
			xlog_payload_print_BINARY( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
			xlog_payload_destory( &payload );
		} else {
			log_r( "failed to create payload\n" );
		}
	}
	
	return 0;
}
