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
	int i = 5; // random() % 1000;
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

int main( int argc, char **argv )
{
	( void )argc;
	( void )argv;
	
	g_mod = xlog_module_open( "/net", XLOG_LEVEL_DEBUG, NULL );
	
	unsigned int count_limit = 10000000;
	unsigned int time_limit = 5;
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(STDER): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(STDER) = %u\n", i/time_limit );
	}
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(STDOT): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(STDOT) = %u\n", i/time_limit );
	}
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "rotating.txt", 1024 * 8, 16 );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(ROTAT): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(ROTAT) = %u\n", i/time_limit );
	}
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "basic-file.txt" );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(BASIC): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(BASIC) = %u\n", i/time_limit );
	}
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "daily-file.txt" );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(DAILY): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(DAILY) = %u\n", i/time_limit );
	}
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_RINGBUF, 1024 * 1024 * 8 );
		time_t st = time( NULL );
		unsigned int i = 0;
		while( time( NULL ) - st < time_limit && i < count_limit ) {
			log_w( "test info(RBUFF): upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf" );
			i ++;
		}
		xlog_test_multi_thread( 10 );
		
		xlog_printer_destory( g_printer );
		fprintf(stderr, "count(RBUFF) = %u\n", i/time_limit );
	}
	#endif
	
	return 0;
}
