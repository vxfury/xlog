#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define XLOG_PRINTER	__printer

#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static int shell_make_args( char *, int *, char **, int );

/*
system
  kernel
  logging
  database
network
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

static xlog_printer_t *__printer = NULL;

static xlog_module_t *m_system = NULL, *m_system_db = NULL, *m_system_kernel = NULL, *m_system_logging = NULL;
static xlog_module_t *m_network = NULL, *m_network_http = NULL,  *m_network_dhcp = NULL;
static xlog_module_t *m_audio_mp3 = NULL,  *m_audio_wav = NULL;
static xlog_module_t *m_video_h264 = NULL,  *m_video_h265 = NULL;

#if !(defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
static xlog_t *g_master = NULL;
#define XLOG_CONTEXT g_master
#define ROOT_MODULE (g_master ? (g_master->module) : NULL)
#else
#define ROOT_MODULE NULL
#endif

#define XLOG_MODULE m_network_dhcp

int main( int argc, char **argv )
{
	__printer = xlog_printer_create( XLOG_PRINTER_STDERR );
	xlog_printer_set_default( __printer );
	XLOG_SET_THREAD_NAME( "thread-xlog" );
	#if !(defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	g_master = xlog_open( "./xlog-nodes", 0 );
	#endif
	{
		m_system = xlog_module_open( "system", XLOG_LEVEL_WARN, ROOT_MODULE );
		m_system_kernel = xlog_module_open( "/system/kernel", XLOG_LEVEL_WARN, ROOT_MODULE );
		m_system_db = xlog_module_open( "db", XLOG_LEVEL_FATAL, m_system );
		m_system_logging = xlog_module_open( "/system/logging", XLOG_LEVEL_ERROR, ROOT_MODULE );
		
		m_network = xlog_module_open( "network", XLOG_LEVEL_INFO, ROOT_MODULE );
		m_network_http = xlog_module_open( "/network/http", XLOG_LEVEL_INFO, ROOT_MODULE );
		m_network_dhcp = xlog_module_open( "dhcp", XLOG_LEVEL_DEBUG, m_network );
		
		m_audio_mp3 = xlog_module_open( "/media/.audio/mp3", XLOG_LEVEL_INFO, ROOT_MODULE );
		m_audio_wav = xlog_module_open( "/media/.audio/wav", XLOG_LEVEL_INFO, ROOT_MODULE );
		
		m_video_h264 = xlog_module_open( "/media/video/h264", XLOG_LEVEL_VERBOSE, ROOT_MODULE );
		m_video_h265 = xlog_module_open( "/media/video/h265", XLOG_LEVEL_DEBUG, ROOT_MODULE );
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
	
	{
		char cmdline[] = "debug -r -L=w /net";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );

		#undef XLOG_MODULE
		#define XLOG_MODULE m_network_http
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
		char cmdline[] = "debug --tag -a -l";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug --no-tag -a -l";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug --only -L=fatal -r /network";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug -f --only -L=fat -r /network";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug -F --only -L=fat -r /network";
		int targc;
		char *targv[10];
		shell_make_args( cmdline, &targc, targv, 10 );
		xlog_shell_main( XLOG_CONTEXT, targc, targv );
	}
	
	{
		char cmdline[] = "debug --only -L -r /network";
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
	
	xlog_module_set_level( ROOT_MODULE, XLOG_LEVEL_VERBOSE, XLOG_LEVEL_ORECURSIVE | XLOG_LEVEL_OFORCE );
	
	xlog_list_modules( NULL, XLOG_LIST_OWITH_TAG | XLOG_LIST_OALL );
	
	xlog_printer_destory( __printer );
	
	return 0;
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
