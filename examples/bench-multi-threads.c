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
	unsigned int nthread = 8;
	unsigned int time_limit = 1;
	unsigned int count_limit = 64;
	char filename[32] = { 0 };
	snprintf( filename, 32, "./logs/bench-multi-threads-%d.txt", nthread );
	FILE *fp = fopen( filename, "w" );
	
	// NOTE: non-buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
			XLOG_ASSERT( g_printer );
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
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
			XLOG_ASSERT( g_printer );
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
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "./logs/file-rotating.txt", 1024 * 8, 16 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "FILE-ROTATE",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC, "./logs/basic-file.txt" );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "FILE-BASIC",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY, "./logs/daily-file.txt" );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "FILE-DAILY",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of FILE-DAILY\n" );
	}
	
	// NOTE: printers with ring-buffer
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "RINGBUF-STDERR",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_RINGBUF, 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "RINGBUF-STDOUT",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-STDOUT\n" );
		
		// should be same as RINGBUF_STDOUT
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_RINGBUF, 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
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
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "RINGBUF-FILE-ROTATE",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-basic.txt", 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "RINGBUF-FILE-BASIC",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_RINGBUF, "./logs/ringbuf-file-daily.txt", 1024 * 1024 * 8 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "RINGBUF-FILE-DAILY",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of RINGBUF-FILE-DAILY\n" );
	}
	
	// NOTE: no-copying buffering printers
	{
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-STDOUT",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDERR | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-STDERR",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDERR\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT | XLOG_PRINTER_BUFF_NCPYRBUF, 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-STDOUT",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-STDOUT\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-rotating.txt", 1024 * 8, 16, 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-FILE-ROTATE",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-ROTATE\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_BASIC | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-basic.txt", 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-FILE-BASIC",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-BASIC\n" );
		
		{
			g_printer = xlog_printer_create( XLOG_PRINTER_FILES_DAILY | XLOG_PRINTER_BUFF_NCPYRBUF, "./logs/no-copy-ringbuf-file-daily.txt", 1024 );
			XLOG_ASSERT( g_printer );
			bench_param_t param = {
				.brief = "NCPY-RINGBUF-FILE-DAILY",
				.printer = g_printer,
				.time_limit = time_limit,
				.count_limit = count_limit,
				.fp = fp,
			};
			xlog_test_multi_thread( nthread, &param );
			xlog_printer_destory( g_printer );
			g_printer = NULL;
		}
		fprintf(stderr, "End of NCPY-RINGBUF-FILE-DAILY\n" );
	}
	xlog_close( NULL, 0 );
	fclose( fp );
	
	return 0;
}
