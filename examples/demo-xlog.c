#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

// #define XLOG_PRINTER	g_printer

#include <xlog/xlog.h>
#include <xlog/xlog_payload.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

unsigned long random_integer( void );
int shell_make_args( char *, int *, char **, int );

/*
sys
  database
  kernel
  ipc
net
  http
  dhcp
media
  audio
    mp3
    wav
  video
    h264
    h265
 */

static xlog_printer_t *g_printer = NULL;

static xlog_module_t *g_sys = NULL, *g_sys_db = NULL, *g_sys_kern = NULL, *g_sys_ipc = NULL;
static xlog_module_t *g_net = NULL, *g_net_http = NULL,  *g_net_dhcp = NULL;
static xlog_module_t *g_audio_mp3 = NULL,  *g_audio_wav = NULL;
static xlog_module_t *g_video_h264 = NULL,  *g_video_h265 = NULL;

#if !(defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
static xlog_t *g_master = NULL;
#define XLOG_CONTEXT g_master
#define ROOT_MODULE (g_master ? (g_master->module) : NULL)
#else
#define ROOT_MODULE NULL
#endif

#define XLOG_MODULE g_net_dhcp

static struct {
	pthread_mutex_t lock;
	struct timeval bench_tv_start;
	struct timeval bench_tv_end;
	size_t log_items;
	size_t log_bytes;
} bench_stats = {
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.log_items = 0,
	.log_bytes = 0,
};

#define BENCH_START() { gettimeofday( &bench_stats.bench_tv_start, NULL ); }
#define BENCH_RECORD(bytes) { pthread_mutex_lock( &bench_stats.lock ); bench_stats.log_items ++; bench_stats.log_bytes += bytes; pthread_mutex_unlock( &bench_stats.lock ); }
#define BENCH_STATS() { \
	gettimeofday( &bench_stats.bench_tv_end, NULL ); \
	unsigned long tv_msec = 1000 * ( bench_stats.bench_tv_end.tv_sec - bench_stats.bench_tv_start.tv_sec ) \
		+ ( bench_stats.bench_tv_end.tv_usec - bench_stats.bench_tv_start.tv_usec ) / 1000; \
	fprintf(stderr, "time elapse: %lu ms, logging: %zu bytes, %zu items\n", tv_msec, bench_stats.log_bytes, bench_stats.log_items ); \
}

/*
static xlog_module_t *g_sys = xlog_module_open
#define XLOG_MODULE g_sys
*/

static int xlog_test_init( void )
{
	g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "rotating.txt", 1024 * 1024 * 5, 64 );
	XLOG_SET_THREAD_NAME( "thread-xlog" );
	#if !(defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	g_master = xlog_open( "./xlog-nodes", 0 );
	#endif
	{
		g_sys = xlog_module_open( "sys", XLOG_LEVEL_WARN, ROOT_MODULE );
		g_sys_kern = xlog_module_open( "/sys/kern", XLOG_LEVEL_WARN, ROOT_MODULE );
		g_sys_db = xlog_module_open( "db", XLOG_LEVEL_FATAL, g_sys );
		g_sys_ipc = xlog_module_open( "/sys/ipc", XLOG_LEVEL_ERROR, ROOT_MODULE );
	}
	
	{
		g_net = xlog_module_open( "net", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_net_http = xlog_module_open( "/net/http", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_net_dhcp = xlog_module_open( "dhcp", XLOG_LEVEL_DEBUG, g_net );
		fprintf(stderr, "level now is %d\n", g_net_dhcp->level );
		xlog_module_t *g_net_dhcp_cpy = xlog_module_open( "/net/dhcp", XLOG_LEVEL_VERBOSE, ROOT_MODULE );
		assert( g_net_dhcp == g_net_dhcp_cpy );
		fprintf(stderr, "level after reopen is %d\n", g_net_dhcp->level );
	}
	
	{
		g_audio_mp3 = xlog_module_open( "/media/audio/mp3", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_audio_wav = xlog_module_open( "/media/audio/wav", XLOG_LEVEL_INFO, ROOT_MODULE );
		
		g_video_h264 = xlog_module_open( "/media/video/H264", XLOG_LEVEL_VERBOSE, ROOT_MODULE );
		g_video_h265 = xlog_module_open( "/media/video/H265", XLOG_LEVEL_DEBUG, ROOT_MODULE );
	}
	
	#define TEST_BUFFER_SIZE		64
	{
		char buffer[TEST_BUFFER_SIZE] = { 0 };
		for( int i = 0; i < sizeof( buffer ); ++ i ) {
			buffer[i] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[random_integer() % (10 + 26 * 2)];
		}
		buffer[sizeof(buffer) - 1] = '\0';
		
		log_r( "raw log output test: %s\n", buffer );
		
		log_i( "fmt log output test: %s", buffer );
	}
	
	{
		char version[32];
		xlog_version( version, 32 );
		log_e( "Logger version : %s\n", version );
		
		log_i( "list modules test" );
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG | XLOG_LIST_OONLY_DROP );
		
		log_r( "\n\n" );
		log_i( "list modules test" );
		xlog_module_list_submodules( ROOT_MODULE, XLOG_LIST_OWITH_TAG );
	}
	
	{
		xlog_module_t *layfind = xlog_module_lookup( ROOT_MODULE, "/net/http" );
		
		log_e( "modulename = %s\n", xlog_module_name( NULL, 0, layfind ) );
	}
	
	{
		xlog_module_set_level( xlog_module_lookup( ROOT_MODULE, "media/audio" ), XLOG_LEVEL_VERBOSE, XLOG_LEVEL_ORECURSIVE | XLOG_LEVEL_OFORCE );
	}
	
	{
		xlog_module_close( g_net_http );
		g_net_http = NULL;
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
		
		g_net_http = xlog_module_open( "/net/http", XLOG_LEVEL_INFO, ROOT_MODULE );
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
	}
	
	xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_VERBOSE, XLOG_LEVEL_ORECURSIVE );
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
	
	return 0;
}

/** BENCH/DEMO: multi-thread */
static void *xlog_test_thread( void *arg )
{
	#define TEST_LOG_INTERVAL (10)
	#define TEST_LOG_TIMECTL (random() % TEST_LOG_INTERVAL)
	int i = 5000; // random() % 1000;
	char name[16];
	snprintf( name, sizeof( name ), "thread-%d", (int)(intptr_t)arg );
	XLOG_SET_THREAD_NAME( name );
	
	int len = 0;
	while( ( i -- ) > 0 ) {
		len = log_v( "verbose: use to trace variables, usually you won't use this level." );
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
		
		len = log_d( "debug: keep silent in release edition." );
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
		
		len = log_i(
		    "info: part of the products, just like those words on the interactive interface.\n"
		    "Feedback of current system status and/or action, so make sure it's significative and intelligible.\n"
		    "Usaully meaningful event information, such as user login/logout, program startup/exit event, request event, etc."
		);
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
		
		len = log_w(
		    "warn: running status that is not expected but can continue to process, so you'd better check and fix it.\n"
		    "Usaully it's owing to improper calling of interface, such as a program invoking an interface that is about to expire, improper parameters, etc."
		);
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
		
		len = log_e( "error: runing time error, current process is terminated." );
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
		
		len = log_f( "fatal: the service/system is not online now." );
		BENCH_RECORD( len );
		usleep( TEST_LOG_TIMECTL );
	}
	
	return NULL;
}

static int xlog_test_multi_thread( int nthread )
{
	pthread_t *threads = (pthread_t *)alloca( sizeof( pthread_t ) * nthread );
	if( threads == NULL ) {
		return -1;
	}
	
	BENCH_START();
	for( int i = 0; i < nthread; i ++ ) {
		pthread_create( threads + i, NULL, xlog_test_thread, (void *)(uintptr_t)i );
	}
	
	for( int i = 0; i < nthread; i ++ ) {
		pthread_join( threads[i], NULL );
	}
	BENCH_STATS();
	
	return 0;
}

/** BENCH/DEMO: level control */
static int xlog_test_set_level( void )
{
	{
		log_r( "Testcase: level control with recursive option\n" );
		
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
		xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_INFO, XLOG_LEVEL_ORECURSIVE );
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
	}
	
	{
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
		
		#undef XLOG_MODULE
		#define XLOG_MODULE g_net
		log_v( "Never Print" );
		xlog_module_set_level( g_net, XLOG_LEVEL_VERBOSE, XLOG_LEVEL_OFORCE | XLOG_LEVEL_ORECURSIVE );
		log_v( "Should Print" );
		
		xlog_list_modules( XLOG_CONTEXT, XLOG_LIST_OWITH_TAG );
	}
	
	{
		log_r( "Testcase: level control with force option\n" );
		
		xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_INFO, XLOG_LEVEL_ORECURSIVE );
		
		#undef XLOG_MODULE
		#define XLOG_MODULE g_net_http
		xlog_module_set_level( g_net_http, XLOG_LEVEL_DEBUG, 0 );
		log_d( "Never Print" );
		
		xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_INFO, XLOG_LEVEL_ORECURSIVE );
		
		#undef XLOG_MODULE
		#define XLOG_MODULE g_net_http
		xlog_module_set_level( g_net_http, XLOG_LEVEL_DEBUG, XLOG_LEVEL_OFORCE );
		log_d( "Should Print" );
	}
	
	{
		log_r( "Testcase: level control via CLI\n" );
		xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_INFO, XLOG_LEVEL_ORECURSIVE );
		int targc;
		char *targv[10];
		
		{
			char cmdline[] = "debug -r -l=w /net";
			shell_make_args( cmdline, &targc, targv, 10 );
			xlog_shell_main( XLOG_CONTEXT, targc, targv );

			#undef XLOG_MODULE
			#define XLOG_MODULE g_net_http
			log_d( "Never Print" );
			log_w( "Should Print" );
		}
		
		{
			char cmdline[] = "debug --version";
			shell_make_args( cmdline, &targc, targv, 10 );
			xlog_shell_main( XLOG_CONTEXT, targc, targv );
		}
	}
	
	return 0;
}

/** BENCH: build rate */
static int xlog_bench_rate( int bench_time )
{
	#define BENCH_BUFFER_SIZE	(64)
	size_t count1 = 0, count2 = 0;
	#define BENCH_TIME		bench_time
	{
		#undef XLOG_MODULE
		#define XLOG_MODULE g_net_http
		char buffer[BENCH_BUFFER_SIZE] = "message";
		for( int i = 0; i < sizeof( buffer ); ++ i ) {
			buffer[i] = 'a' + random_integer() % 26;
		}
		buffer[sizeof(buffer) - 1] = '\0';

		time_t ts = time( NULL );
		size_t count = 0;
		while( time(NULL) - ts <= BENCH_TIME )
		{
			log_w("%s", buffer);
			count ++;
		}
		count1 = count / BENCH_TIME;
		fprintf(stderr, "count = %zu\n", count );
	}
	
	{
		char buffer[BENCH_BUFFER_SIZE] = "message";
		for( int i = 0; i < sizeof( buffer ); ++ i ) {
			buffer[i] = 'a' + random_integer() % 26;
		}
		buffer[sizeof(buffer) - 1] = '\0';

		time_t ts = time( NULL );
		size_t count = 0;
		while( time(NULL) - ts <= BENCH_TIME )
		{
			char *temp = (char *)malloc( 2048 );
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
			        XLOG_TAG_PREFIX_LOG_TIME "%02d/%02d %02d:%02d:%02d.%03d" XLOG_TAG_SUFFIX_LOG_TIME XLOG_TAG_PREFIX_LOG_CLASS(WARN) "%s" XLOG_TAG_SUFFIX_LOG_CLASS(WARN) "%s",
			        p->tm_mon + 1, p->tm_mday,
			        p->tm_hour, p->tm_min, p->tm_sec, ( int )( now.tv.tv_usec / 1000 ),
			        "/net/dhcp",
			        buffer
			    );
			    #ifndef XLOG_BENCH_NO_OUTPUT
			    printf("%s\n", temp);
			    #endif
			    free(temp);
				count ++;
			}
		}
		count2 = count / BENCH_TIME;
		fprintf(stderr, "count = %zu\n", count );
	}

	fprintf(stderr, "count1 = %zu(%f), count2 = %zu\n", count1, (double)count1/(double)count2, count2 );
	
	return 0;
}

char *xlog_path_format( char *buffer, size_t length, const char *path );

int main( int argc, char **argv )
{
	xlog_test_init();
	xlog_test_set_level();
	// xlog_test_multi_thread( 10 );
	// xlog_bench_rate( 5 );
	
	#if 0
	BENCH_START();
	for( int i = 0; i < 100000; i ++ ) {
		int len = log_v( "cwnkewrnervejve jvebe" );
		BENCH_RECORD(len);
	}
	BENCH_STATS();
	#endif
	
	xlog_close( xlog_module_context( ROOT_MODULE ), 0 );
	
	xlog_printer_destory( g_printer );
	
	return 0;
}

unsigned long random_integer( void )
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
		
		srand( (unsigned int)seed );
		initialized = true;
	}
	
	return rand();
}

int shell_make_args( char *cmdline, int *argc_p, char **argv_p, int max_args )
{
	assert( cmdline );
	
	int status = 0, argc = 0;
	char *ch = cmdline;
	
	while( *ch ) {
		while( isspace( ( unsigned char )*ch ) ) {
			ch ++;
		}
		
		if( *ch == '\0' ) {
			break;
		}
		
		if( *ch == '"' ) {
			argv_p[argc] = ++ch;
			while( ( *ch != '\0' ) && ( *ch != '"' ) ) {
				ch ++;
			}
		} else {
			argv_p[argc] = ch;
			while( ( *ch != '\0' ) && ( !isspace( ( unsigned char )*ch ) ) ) {
				ch++;
			}
		}
		argc ++;
		
		if( *ch == '\0' ) {
			break;
		}
		*ch++ = '\0';
		
		if( argc == ( max_args - 1 ) ) {
			status = -1;
			break;
		}
	}
	argv_p[argc] = NULL;
	*argc_p = argc;
	
	return status;
}
