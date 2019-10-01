#undef XLOG_PRINTER
#define XLOG_PRINTER	g_printer

#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static xlog_printer_t *g_printer = NULL;
static xlog_module_t *g_mod = NULL;
#define XLOG_MODULE g_mod

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
	
	g_mod = xlog_module_open( "/net", XLOG_LEVEL_DEBUG, NULL );
	
	#define BENCH_BUFFER_SIZE 64
	char buffer[BENCH_BUFFER_SIZE];
	for( int i = 0; i < sizeof( buffer ); ++ i ) {
		buffer[i] = 'a' + random_integer() % 26;
	}
	buffer[sizeof( buffer ) - 1] = '\0';
	
	unsigned int count_limit = 80;
	// NOTE: non-buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "./logs/rotating.txt", 1024 * 8, 16 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "./logs/basic-file.txt" );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "./logs/daily-file.txt" );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-DAILY\n" );
	}
	
	// NOTE: printers with ring-buffer
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-STDOUT\n" );
		
		// should be same as RINGBUF_STDOUT
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_RINGBUF, 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RING-BUFFER\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-basic.txt", 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-daily.txt", 1024 * 1024 * 8 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-DAILY\n" );
	}
	
	// NOTE: no-copying buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-basic.txt", 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-daily.txt", 1024 );
			unsigned int i = 0;
			while( i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-DAILY\n" );
	}
	
	// NULL module and empty thread name test
	{
		#undef XLOG_MODULE
		#define XLOG_MODULE NULL
		log_i( "test info" );
		
		XLOG_SET_THREAD_NAME( "cov-printer" );
		log_i( "test info" );
	}
	
	return 0;
}
