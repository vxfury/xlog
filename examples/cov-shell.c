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

int main( int argc, char **argv )
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
		
		g_net = xlog_module_open( "net", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_net_http = xlog_module_open( "/net/http", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_net_dhcp = xlog_module_open( "dhcp", XLOG_LEVEL_DEBUG, g_net );
		
		g_audio_mp3 = xlog_module_open( "/media/.audio/mp3", XLOG_LEVEL_INFO, ROOT_MODULE );
		g_audio_wav = xlog_module_open( "/media/.audio/wav", XLOG_LEVEL_INFO, ROOT_MODULE );
		
		g_video_h264 = xlog_module_open( "/media/video/H264", XLOG_LEVEL_VERBOSE, ROOT_MODULE );
		g_video_h265 = xlog_module_open( "/media/video/H265", XLOG_LEVEL_DEBUG, ROOT_MODULE );
	}
	
	{
		char cmdline[] = "debug --version";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug -h";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_INFO, XLOG_LEVEL_ORECURSIVE );
	{
		char cmdline[] = "debug -r -L=w /net";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );

		#undef XLOG_MODULE
		#define XLOG_MODULE g_net_http
		log_d( "Never Print" );
		log_w( "Should Print" );
	}
	
	{
		char cmdline[] = "debug -l";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug -a -l";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	xlog_printer_destory( g_printer );
	
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
