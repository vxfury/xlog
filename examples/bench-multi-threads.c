#undef XLOG_PRINTER
#define XLOG_PRINTER	g_printer

#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

typedef struct {
	const char *brief;
	FILE *fp;
	xlog_printer_t *printer;
	unsigned int time_limit;
	unsigned int count_limit;
} bench_param_t;

/** BENCH/DEMO: multi-thread */
static void *xlog_test_thread( void *arg )
{
	char name[16];
	bench_param_t *param = (bench_param_t *)arg;
	snprintf( name, sizeof( name ), "thread-%s", param->brief );
	XLOG_SET_THREAD_NAME( name );
	#define __log(...)	xlog_output_fmtlog( param->printer, NULL, XLOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__ )
	int i = 0;
	time_t ts = time( NULL );
	while( time( NULL ) - ts < param->time_limit && i < param->count_limit ) {
		__log( "verbose: use to trace variables, usually you won't use this level." );
		
		#if 0
		__log( "debug: keep silent in release edition." );
		
		__log(
		    "info: part of the products, just like those words on the interactive interface.\n"
		    "Feedback of current system status and/or action, so make sure it's significative and intelligible.\n"
		    "Usaully meaningful event information, such as user login/logout, program startup/exit event, request event, etc."
		);
		
		__log(
		    "warn: running status that is not expected but can continue to process, so you'd better check and fix it.\n"
		    "Usaully it's owing to improper calling of interface, such as a program invoking an interface that is about to expire, improper parameters, etc."
		);
		
		__log( "error: runing time error, current process is terminated." );
		
		__log( "fatal: the service/system is not online now." );
		#endif
		
		i ++;
	}
	
	return (void *)(intptr_t)i;
}

static int xlog_test_multi_thread( int nthread, bench_param_t *param )
{
	pthread_t *threads = ( pthread_t * )alloca( sizeof( pthread_t ) * nthread );
	if( threads == NULL ) {
		return -1;
	}
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
	for( int i = 0; i < nthread; i ++ ) {
		pthread_create( threads + i, &attr, xlog_test_thread, ( void * )param );
	}
	pthread_attr_destroy( &attr );
	
	int total = 0;
	void *count = NULL;
	for( int i = 0; i < nthread; i ++ ) {
		pthread_join( threads[i], &count );
		total += (int)(intptr_t)count;
	}
	fprintf( param->fp, "\nBrief(%s): %d\n", param->brief, total );
	
	return 0;
}

int main( int argc, char **argv )
{
	( void )argc;
	( void )argv;
	xlog_printer_t *g_printer = NULL;
	unsigned int nthread = 16;
	unsigned int time_limit = 5;
	unsigned int count_limit = 10;
	char filename[32] = { 0 };
	snprintf( filename, 32, "bench-multi-threads-%d.txt", nthread );
	FILE *fp = fopen( filename, "w" );
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
		
		bench_param_t param = {
			.brief = "STDERR",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of STDERR\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
		bench_param_t param = {
			.brief = "STDOUT",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of STDOUT\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "rotating.txt", 1024 * 8, 16 );
		bench_param_t param = {
			.brief = "ROTATING-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of ROTATING-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "basic-file.txt" );
		bench_param_t param = {
			.brief = "BASIC-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of BASIC-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "daily-file.txt" );
		bench_param_t param = {
			.brief = "DAILY-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of DAILY-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_RINGBUF, 1024 * 1024 * 8 );
		bench_param_t param = {
			.brief = "RING-BUFFER",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RING-BUFFER\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_RINGBUF );
		bench_param_t param = {
			.brief = "RB-STDERR",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RB-STDERR\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_RINGBUF );
		bench_param_t param = {
			.brief = "RB-STDOUT",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RB-STDOUT\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_RINGBUF, "rotating-rb.txt", 1024 * 8, 16 );
		bench_param_t param = {
			.brief = "RB-ROTATING-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RB-ROTATING-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_RINGBUF, "basic-file-rb.txt" );
		bench_param_t param = {
			.brief = "RB-BASIC-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RB-BASIC-FILE\n" );
	#endif
	
	#if 1
	{
		g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_RINGBUF, "daily-file-rb.txt" );
		bench_param_t param = {
			.brief = "RB-DAILY-FILE",
			.printer = g_printer,
			.time_limit = time_limit,
			.count_limit = count_limit,
			.fp = fp,
		};
		xlog_test_multi_thread( nthread, &param );
		xlog_printer_destory( g_printer );
		g_printer = NULL;
	}
	fprintf(stderr, "End of RB-DAILY-FILE\n" );
	#endif
	
	fclose( fp );
	
	return 0;
}
