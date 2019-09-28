#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static int random_integer( void )
{
	static bool initialized = false;
	if( !initialized ) {
		unsigned int seed;
		
		int fd = open( "/dev/urandom", O_RDONLY );
		if( fd < 0 || read( fd, &seed, sizeof( seed ) ) < 0 ) {
			seed = time( NULL );
		}
		if( fd >= 0 ) {
			close( fd );
			fd = -1;
		}
		
		srand( seed );
		initialized = true;
	}
	
	return rand();
}

int main( int argc, char **argv )
{
	( void )argc;
	( void )argv;
	
	{
		autobuf_t *payload = autobuf_create(
		    PAYLOAD_ID_TEXT, "Text",
		    AUTOBUF_ODYNAMIC | AUTOBUF_OALIGN | AUTOBUF_OTEXT,
		    1024, 32
		);
		
		char buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer() % 26 + 'a';
		}
		buff[sizeof( buff ) - 1] = '\0';
		
		autobuf_append_text( &payload, buff );
		payload_print_TEXT( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
		autobuf_destory( &payload );
		
		log_r( "\n\nbuffer = %s\n", buff );
	}
	
	{
		autobuf_t *payload = autobuf_create(
		    PAYLOAD_ID_BINARY, "Binary",
		    AUTOBUF_ODYNAMIC | AUTOBUF_OALIGN | AUTOBUF_OBINARY,
		    1024, 32
		);
		
		int buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer();
		}
		
		autobuf_append_binary( &payload, buff, sizeof( buff ) );
		payload_print_BINARY( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
		autobuf_destory( &payload );
	}
	
	{
		char buffer[1024];
		autobuf_t *payload = autobuf_create(
		    PAYLOAD_ID_BINARY, "Binary",
		    AUTOBUF_OFIXED | AUTOBUF_OBINARY,
		    buffer, sizeof( buffer )
		);
		
		if( payload ) {
			int buff[128];
			for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
				buff[i] = random_integer();
			}
			
			autobuf_append_binary( &payload, buff, sizeof( buff ) );
			payload_print_BINARY( payload, xlog_printer_create( XLOG_PRINTER_STDOUT ) );
			autobuf_destory( &payload );
		} else {
			log_r( "failed to create payload\n" );
		}
	}
	
	return 0;
}
