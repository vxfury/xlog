#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "internal/xlog.h"
#include "internal/xlog_tree.h"
#include "internal/xlog_hexdump.h"

#include <getopt.h>
#include <setjmp.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#ifndef __APPLE__
#pragma GCC diagnostic ignored "-Wclobbered"
#endif
#endif

typedef struct {
	xlog_t *context;
	
	int level;
	int f_force;
	int f_traverse;
	int f_only;
	int f_with_tag;
	
	int exit_code;
	jmp_buf exit_jmp;
} xlog_shell_globals_t;

#define LOG_CLI_OPT_BASE        128
#define LOG_CLI_OPT_LIST        (LOG_CLI_OPT_BASE + 0)
#define LOG_CLI_OPT_ONLY		(LOG_CLI_OPT_BASE + 1)
#define LOG_CLI_OPT_WITH_TAG	(LOG_CLI_OPT_BASE + 2)
#define LOG_CLI_OPT_WITHOUT_TAG	(LOG_CLI_OPT_BASE + 3)

static const struct option debug_options[] = {
	{ "force"			, no_argument		, NULL	, 'f'						},
	{ "traverse"		, no_argument		, NULL	, 'r'						},
	{ "level"			, required_argument	, NULL	, 'l'						},
	{ "list"			, no_argument		, NULL	, LOG_CLI_OPT_LIST			},
	{ "only"			, no_argument		, NULL	, LOG_CLI_OPT_ONLY			},
	{ "with-tag"		, no_argument		, NULL	, LOG_CLI_OPT_WITH_TAG		},
	{ "without-tag"		, no_argument		, NULL	, LOG_CLI_OPT_WITHOUT_TAG	},
	{ "version"			, no_argument		, NULL	, 'v'						},
	{ "help"			, no_argument		, NULL	, 'h'						},
	{ NULL				, 0					, NULL	, '\0'						}
};
#define XLOG_CLI_SHORT_OPTIONS	"fFrl:vh"

static void shell_debug_exit( xlog_shell_globals_t *globals, int code )
{
	globals->exit_code = code;
	longjmp( globals->exit_jmp, 1 );
}
#define exit( code )    shell_debug_exit( globals, code )

static void usage( xlog_shell_globals_t *globals )
{
	( void )fprintf(
	    stderr,
	    "Usage: debug [OPTIONS] MODULE[/SUB-MODULE/...]\n"
	    "\n"
	    "Mandatory arguments to long options are mandatory for short options too.\n"
	    "  -f, --force        force enable module debugging.\n"
	    "  -F                 revert --force option.\n"
	    "  -r, --traverse     traverse enable module debugging.\n"
	    "  -l, --level=LEVEL  debugging level. XLOG_LEVEL_DEBUG if not specified.\n"
	    "                     s[ilent]/f[atal]/e[rror]/w[arn]/i[nfo]/d[ebug]/v[erbose](case insensitive, or -1 ~ 5).\n"
	    "  --list             list modules in your application.\n"
	    "  --only             enable xlog output of specified modules(disabling will be applied to other modules).\n"
	    "  -v, --version      show version of logger.\n"
	    "  -h, --help         display this help and exit.\n"
	    "\n"
	);
	exit( EXIT_FAILURE );
	/* NOTREACHED */
}

static int main_debug( xlog_shell_globals_t *globals, int argc, char **argv )
{
	int ch, oi;
	struct getopt_data getopt_reent;
	memset( &getopt_reent, 0, sizeof( getopt_data ) );
	
	globals->level = XLOG_LEVEL_DEBUG;
	globals->f_force = 1;
	globals->f_with_tag = XLOG_LIST_OWITH_TAG;
	
	while( (
        ch = getopt_long_r(
            argc, argv,
            XLOG_CLI_SHORT_OPTIONS, debug_options, &oi, &getopt_reent
        )
	) != - 1 ) {
		switch( ch ) {
			case 'f':
				globals->f_force = 1;
				break;
			case 'F':
				globals->f_force = 0;
				break;
			case 'r':
				globals->f_traverse = 1;
				break;
			case 'l':
				if( getopt_reent.optarg && *( getopt_reent.optarg ) != '\0' ) {
					#define __X( b ) (0 == strcasecmp( getopt_reent.optarg, b ))
					#define __Y( num, tag, word ) ( __X( tag ) || __X( num ) || __X( word ) )
					if( __Y( "0", "f", "fatal" ) ) {
						globals->level = XLOG_LEVEL_FATAL;
					} else if( __Y( "1", "e", "error" ) ) {
						globals->level = XLOG_LEVEL_ERROR;
					} else if( __Y( "2", "w", "warn" ) ) {
						globals->level = XLOG_LEVEL_WARN;
					} else if( __Y( "3", "i", "info" ) ) {
						globals->level = XLOG_LEVEL_INFO;
					} else if( __Y( "4", "d", "debug" ) ) {
						globals->level = XLOG_LEVEL_DEBUG;
					} else if( __Y( "5", "v", "verbose" ) ) {
						globals->level = XLOG_LEVEL_VERBOSE;
					} else if( __Y( "-1", "s", "silent" ) ) {
						globals->level = XLOG_LEVEL_SILENT;
					}
					#undef __Y
					#undef __X
				} else {
					usage( globals );
					exit( EXIT_FAILURE );
				}
				break;
			case LOG_CLI_OPT_WITH_TAG:
				globals->f_with_tag = 1;
				break;
			case LOG_CLI_OPT_WITHOUT_TAG:
				globals->f_with_tag = 0;
				break;
			case 'v': {
				char _version[32];
				xlog_version( _version, sizeof( _version ) );
				log_r( "Logger version %s\n", _version );
				exit( EXIT_SUCCESS );
			} break;
			case 'h':
				usage( globals );
				exit( EXIT_SUCCESS );
				break;
			case LOG_CLI_OPT_LIST:
				log_r( "logging modules in your application:\n" );
				xlog_list_modules( globals->context, globals->f_with_tag );
				log_r( "\n" );
				exit( EXIT_SUCCESS );
				break;
			case LOG_CLI_OPT_ONLY:
				globals->f_only = 1;
				break;
			default:
				exit( EXIT_FAILURE );
				break;
		}
	}
	argc -= getopt_reent.optind;
	argv += getopt_reent.optind;
	
	int flags = 0;
	if( globals->f_traverse ) {
		flags |= XLOG_LEVEL_ORECURSIVE;
	}
	if( globals->f_force ) {
		flags |= XLOG_LEVEL_OFORCE;
	}
	
	if( argc > 0 ) { // settings for multiple modules could be supported
		if( globals->f_only ) {
			globals->exit_code |= xlog_module_set_level(
		        globals->context->module,
		        XLOG_LEVEL_SILENT, XLOG_LEVEL_ORECURSIVE
		    );
		}
		int i;
		for( i = 0; i < argc; i ++ ) {
			globals->exit_code |= xlog_module_set_level(
			        xlog_module_lookup( globals->context->module, argv[i] ),
			        globals->level, flags
			    );
		}
		exit( globals->exit_code );
	} else {
		exit( EXIT_FAILURE );
	}
	
	/* NOTREACHED */
	return 0;
}

/*
 * debug [OPTIONS] mod/submod/...
 *   OPTIONS:
 *      -f, --force        force enable module debugging[default].
 *      -F                 undo force option.
 *      -r, --recursive    apply to sub-modules recursively.
 *      -l, --level=LEVEL  debugging level. XLOG_LEVEL_DEBUG if not specified.
 *                         s[ilent]/f[atal]/e[rror]/w[arn]/i[nfo]/d[ebug]/v[erbose].(-1 ~ 5).
 *      --list             list modules in your project.
 *      -h, --help         display this help and exit.
 */
XLOG_PUBLIC(int) xlog_shell_main( xlog_t *context, int argc, char **argv )
{
	xlog_t *__context = context;
	if( __context == NULL ) {
		#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
		__context = xlog_module_context( NULL );
		if( __context == NULL ) {
			return ENOMEM;
		}
		#else
		return EINVAL;
		#endif
	}
	
	xlog_shell_globals_t globals;
	memset( &globals, 0, sizeof( xlog_shell_globals_t ) );
	
	globals.exit_code = 1;
	if( 0 == setjmp( globals.exit_jmp ) ) {
		globals.context = __context;
		return main_debug( &globals, argc, argv );
	}
	return globals.exit_code;
}
