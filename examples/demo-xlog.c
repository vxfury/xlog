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

#define XLOG_PRINTER	g_printer

#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>
#include <xlog/xlog_debug.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static unsigned long random_integer( void );
static int shell_make_args( char *, int *, char **, int );

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
		fprintf( stderr, "level now is %d\n", g_net_dhcp->level );
		xlog_module_t *g_net_dhcp_cpy = xlog_module_open( "/net/dhcp", XLOG_LEVEL_VERBOSE, ROOT_MODULE );
		assert( g_net_dhcp == g_net_dhcp_cpy );
		fprintf( stderr, "level after reopen is %d\n", g_net_dhcp->level );
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
			buffer[i] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[random_integer() % ( 10 + 26 * 2 )];
		}
		buffer[sizeof( buffer ) - 1] = '\0';
		
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

/** DEMO: level control */
static int xlog_test_set_level( void )
{
	{
		fprintf( stderr, "Testcase: level control with recursive option\n" );
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
			char cmdline[] = "debug -r -L=w /net";
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

int main( int argc, char **argv )
{
	xlog_test_init();
	xlog_test_set_level();
	
	xlog_close( xlog_module_context( ROOT_MODULE ), 0 );
	
	xlog_printer_destory( g_printer );
	
	int i = 10;
	debug( i, i, i, i );
	
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

static int shell_make_args( char *cmdline, int *argc_p, char **argv_p, int max_args )
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
