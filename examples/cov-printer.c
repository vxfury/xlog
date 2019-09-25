#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static xlog_printer_t *g_printer = NULL;
static xlog_module_t *g_mod = NULL;
#define XLOG_MODULE g_mod

#undef XLOG_PRINTER
#define XLOG_PRINTER	g_printer

/** BENCH/DEMO: multi-thread */
static void *xlog_test_thread( void *arg )
{
	int i = 50; // random() % 1000;
	char name[16];
	snprintf( name, sizeof( name ), "thread-%d", ( int )( intptr_t )arg );
	XLOG_SET_THREAD_NAME( name );
	
	while( ( i -- ) > 0 ) {
		log_v( "verbose: use to trace variables, usually you won't use this level." );
		
		log_d( "debug: keep silent in release edition." );
		
		log_i(
		    "info: part of the products, just like those words on the interactive interface.\n"
		    "Feedback of current system status and/or action, so make sure it's significative and intelligible.\n"
		    "Usaully meaningful event information, such as user login/logout, program startup/exit event, request event, etc."
		);
		
		log_w(
		    "warn: running status that is not expected but can continue to process, so you'd better check and fix it.\n"
		    "Usaully it's owing to improper calling of interface, such as a program invoking an interface that is about to expire, improper parameters, etc."
		);
		
		log_e( "error: runing time error, current process is terminated." );
		
		log_f( "fatal: the service/system is not online now." );
	}
	
	return NULL;
}

static int xlog_test_multi_thread( int nthread )
{
	pthread_t *threads = ( pthread_t * )alloca( sizeof( pthread_t ) * nthread );
	if( threads == NULL ) {
		return -1;
	}
	
	for( int i = 0; i < nthread; i ++ ) {
		pthread_create( threads + i, NULL, xlog_test_thread, ( void * )( uintptr_t )i );
	}
	
	for( int i = 0; i < nthread; i ++ ) {
		pthread_join( threads[i], NULL );
	}
	
	return 0;
}

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
	unsigned int count_limit = 10;
	unsigned int time_limit = 1;
	
	unsigned int index = 0;
	struct {
		const char *brief;
		unsigned int count;
	} bench_result[10];
	
	char buffer[BENCH_BUFFER_SIZE];
	for( int i = 0; i < sizeof( buffer ); ++ i ) {
		buffer[i] = 'a' + random_integer() % 26;
	}
	buffer[sizeof( buffer ) - 1] = '\0';
	
	{
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			char *temp = ( char * )malloc( 2048 );
			if( temp ) {
				xlog_time_t now;
				#if ((defined __linux__) || (defined __FreeBSD__) || (defined __APPLE__) || (defined __unix__))
				struct timeval tv;
				struct timezone tz;
				gettimeofday( &tv, &tz );
				now.tv.tv_sec = tv.tv_sec;
				now.tv.tv_usec = tv.tv_usec;
				now.tz.tz_minuteswest = tz.tz_minuteswest;
				now.tz.tz_dsttime = tz.tz_dsttime;
				#else
				#error No implementation for this system.
				#endif
				time_t tv_sec = now.tv.tv_sec;
				struct tm *p = localtime( &tv_sec );
				snprintf(
				    temp, 2048,
				    XLOG_TAG_PREFIX_LOG_TIME_NONE "%02d/%02d %02d:%02d:%02d.%03d" XLOG_TAG_SUFFIX_LOG_TIME_NONE XLOG_TAG_PREFIX_LOG_CLASS_NONE( WARN ) "%s" XLOG_TAG_SUFFIX_LOG_CLASS_NONE( WARN ) "%s",
				    p->tm_mon + 1, p->tm_mday,
				    p->tm_hour, p->tm_min, p->tm_sec, ( int )( now.tv.tv_usec / 1000 ),
				    "/net",
				    buffer
				);
				#ifndef XLOG_BENCH_NO_OUTPUT
				printf( "%s\n", temp );
				#endif
				free( temp );
				i ++;
			}
		}
		
		bench_result[index].brief = "BASE";
		bench_result[index].count = i / time_limit;
		index ++;
	}
	fprintf(stderr, "End of BASE\n" );
	
	#if 1
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
	#endif
	
	#if 1
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
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "rotating.txt", 1024 * 8, 16 );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "%s", buffer );
			i ++;
		}
		xlog_printer_destory( g_printer );
		g_printer = NULL;
		
		bench_result[index].brief = "ROTATING-FILE";
		bench_result[index].count = i / time_limit;
		index ++;
	}
	fprintf(stderr, "End of ROTATING-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "basic-file.txt" );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "%s", buffer );
			i ++;
		}
		xlog_printer_destory( g_printer );
		g_printer = NULL;
		
		bench_result[index].brief = "BASIC-FILE";
		bench_result[index].count = i / time_limit;
		index ++;
	}
	fprintf(stderr, "End of BASIC-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "daily-file.txt" );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "%s", buffer );
			i ++;
		}
		xlog_printer_destory( g_printer );
		g_printer = NULL;
		
		bench_result[index].brief = "DAILY-FILE";
		bench_result[index].count = i / time_limit;
		index ++;
	}
	fprintf(stderr, "End of DAILY-FILE\n" );
	#endif
	
	#if 1
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
	#endif
	
	xlog_test_multi_thread( 10 );
	
	for( int i = 0; i < index; i ++ ) {
		fprintf(stderr, "%16s: %8u(%.3f)\n", bench_result[i].brief, bench_result[i].count, (double)bench_result[i].count / (double)bench_result[0].count );
	}
	
	{
		#undef XLOG_MODULE
		#define XLOG_MODULE NULL
		log_i( "test info" );
		
		XLOG_SET_THREAD_NAME( "cov-printer" );
		log_i( "test info" );
	}
	
	return 0;
}
