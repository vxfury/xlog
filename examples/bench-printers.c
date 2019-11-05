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
	unsigned int count_limit = 10000000;
	unsigned int time_limit = 1;
	
	unsigned int index = 0;
	struct {
		const char *brief;
		unsigned int count;
	} bench_result[20];
	
	char buffer[BENCH_BUFFER_SIZE];
	for( int i = 0; i < sizeof( buffer ); ++ i ) {
		buffer[i] = 'a' + random_integer() % 26;
	}
	buffer[sizeof( buffer ) - 1] = '\0';
	
	// BASE: malloc-snprintf-printf
	{
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			char *temp = ( char * )malloc( 2048 );
			if( temp ) {
				#if ((defined __linux__) || (defined __FreeBSD__) || (defined __APPLE__) || (defined __unix__))
				struct timeval tv;
				struct timezone tz;
				gettimeofday( &tv, &tz );
				struct tm *p = localtime( &tv.tv_sec );
				snprintf(
				    temp, 2048,
				    XLOG_PREFIX_LOG_TIME_NONE "%02d/%02d %02d:%02d:%02d.%03d" XLOG_SUFFIX_LOG_TIME_NONE XLOG_PREFIX_LOG_CLASS_NONE( WARN ) "%s" XLOG_SUFFIX_LOG_CLASS_NONE( WARN ) "%s",
				    p->tm_mon + 1, p->tm_mday,
				    p->tm_hour, p->tm_min, p->tm_sec, ( int )( tv.tv_usec / 1000 ),
				    "/net",
				    buffer
				);
				#ifndef XLOG_BENCH_NO_OUTPUT
				printf( "%s\n", temp );
				#endif
				free( temp );
				i ++;
				#else
				#error No implementation for this system.
				#endif
			}
		}
		
		bench_result[index].brief = "BASE";
		bench_result[index].count = i / time_limit;
		index ++;
	}
	fprintf(stderr, "End of BASE\n" );
	
	// NOTE: non-buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "STDOUT";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "STDERR";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "./logs/file-rotating.txt", 1024 * 8, 16 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "FILE-ROTATE";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "./logs/basic-file.txt" );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "FILE-BASIC";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "./logs/daily-file.txt" );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "FILE-DAILY";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of FILE-DAILY\n" );
	}
	
	// NOTE: printers with ring-buffer
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RINGBUF-STDERR";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RINGBUF-STDOUT";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RINGBUF-STDOUT\n" );
		
		// should be same as RINGBUF_STDOUT
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_RINGBUF, 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RING-BUFFER";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RING-BUFFER\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RINGBUF-FILE-ROTATE";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-basic.txt", 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RINGBUF-FILE-BASIC";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-daily.txt", 1024 * 1024 * 8 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "RINGBUF-FILE-DAILY";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of RINGBUF-FILE-DAILY\n" );
	}
	
	// NOTE: no-copying buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-STDOUT";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-STDERR";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-STDOUT";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-FILE-ROTATE";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-basic.txt", 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-FILE-BASIC";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-daily.txt", 1024 );
			time_t st = time( NULL );
			unsigned int i = 0;
			while( time( NULL ) - st < time_limit && i < count_limit ) {
				log_w( "%s", buffer );
				i ++;
			}
			xlog_printer_destory( g_printer );
			g_printer = NULL;
			
			bench_result[index].brief = "NCPY-RINGBUF-FILE-DAILY";
			bench_result[index].count = i / time_limit;
			index ++;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-DAILY\n" );
	}
	xlog_close( NULL, 0 );
	for( int i = 0; i < index; i ++ ) {
		fprintf(stderr, "%24s: %8u(%.3f)\n", bench_result[i].brief, bench_result[i].count, (double)bench_result[i].count / (double)bench_result[0].count );
	}
	
	return 0;
}
