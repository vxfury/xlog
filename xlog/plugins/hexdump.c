#include <xlog/xlog_config.h>
#include "plugins/hexdump.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#define HEXD_MIN(X,Y) ((X) < (Y)? (X) : (Y))
#define HEXD_ARRAY_SIZE(arr)	(sizeof(arr) / sizeof(*arr))
#define HEXD_LINE_RSV_SIZE_COLORS		32

static const char *CHAR_AREA_HIGH_LUT[] = {
	"€", ".", "‚", "ƒ", "„", "…", "†", "‡", "ˆ", "‰", "Š", "‹", "Œ", ".", "Ž", ".",
	".", "‘", "’", "“", "”", "•", "–", "—", "˜", "™", "š", "›", "œ", ".", "ž", "Ÿ",
	".", "¡", "¢", "£", "¤", "¥", "¦", "§", "¨", "©", "ª", "«", "¬", ".", "®", "¯",
	"°", "±", "²", "³", "´", "µ", "¶", "·", "¸", "¹", "º", "»", "¼", "½", "¾", "¿",
	"À", "Á", "Â", "Ã", "Ä", "Å", "Æ", "Ç", "È", "É", "Ê", "Ë", "Ì", "Í", "Î", "Ï",
	"Ð", "Ñ", "Ò", "Ó", "Ô", "Õ", "Ö", "×", "Ø", "Ù", "Ú", "Û", "Ü", "Ý", "Þ", "ß",
	"à", "á", "â", "ã", "ä", "å", "æ", "ç", "è", "é", "ê", "ë", "ì", "í", "î", "ï",
	"ð", "ñ", "ò", "ó", "ô", "õ", "ö", "÷", "ø", "ù", "ú", "û", "ü", "ý", "þ", "ÿ",
};

static const char *hexdump_format_of( int v )
{
	#if !(defined HEXD_ENABLE_ONLOAD_FORMAT)
	static bool initialized = false;
	#endif
	static struct {
		const char *key;
		char format[16];
	} formats[] = {
		{ "zero", "38;5;238" },
		{ "all", "38;5;167" },
		{ "low", "38;5;150" },
		{ "high", "38;5;141" },
		{ "printable", "" },
	};
	
	#if !(defined HEXD_ENABLE_ONLOAD_FORMAT)
	if( !initialized ) {
		// Parse HEXD_COLORS
		char *colors_var = getenv( "HEXD_COLORS" );
		if( colors_var != NULL ) {
			colors_var = strdup( colors_var );
			if( colors_var == NULL ) {
				errx( 1, "strdup" );
			}
			
			for( char *p = colors_var, *token; ( void )( token = strtok( p, " " ) ), token != NULL; p = NULL ) {
				char *key = token, *value = strchr( token, '=' );
				if( value == NULL ) {
					warnx( "no '=' found in HEXD_COLORS property '%s'", p );
				}
				*value++ = '\0';
				
				int i = (
		            strcmp( key, "zero" ) == 0 ? 0 :
		            strcmp( key, "all"  ) == 0 ? 1 :
		            strcmp( key, "low"  ) == 0 ? 2 :
		            strcmp( key, "high" ) == 0 ? 3 :
		            strcmp( key, "high" ) == 0 ? 4 : -1
		        );
				if( i >= 0 && i < ( int )HEXD_ARRAY_SIZE( formats ) ) {
					initialized = true;
					snprintf( formats[i].format, sizeof( formats[i].format ), "%s", value );
				} else {
					warnx( "unknown HEXD_COLORS property '%s'", key );
				}
			}
			
			free( colors_var );
		}
	}
	#endif
	
	return ( const char * )(
		formats[
		v == 0x00 ? 0 :
		v == 0xFF ? 1 :
		v <  0x20 ? 2 :
		v >= 0x7F ? 4 : 5
		].format
	);
}

int __hexdump_iterator(
    hexdump_iterator_t *iterator,
    const void *target, const hexdump_options_t *options,
    int ( *readline )( const void * /* target */, off_t /* offset */, void * /* buffer */, size_t /* size */ )
)
{
	XLOG_ASSERT( target );
	XLOG_ASSERT( options );
	XLOG_ASSERT( readline );
	XLOG_ASSERT( iterator );
	XLOG_ASSERT( iterator->line );
	XLOG_ASSERT( iterator->dumpline && iterator->dumpsize > 0 );
	
	size_t nread = options->end == -1 ? options->columns : HEXD_MIN( options->columns, ( size_t )( options->end - iterator->offset ) );
	int n = readline( target, iterator->offset, ( void * )iterator->line, nread );
	if( n <= 0 ) {
		iterator->status = HEXD_EOF;
		
		return n;
	}
	iterator->offset += n;
	char *dumpptr = iterator->dumpline;
	
	if(
	    iterator->status != HEXD_FIRST_LINE
	    && 0 == memcmp( iterator->line, iterator->prev_line, options->columns )
	    && n == ( int )options->columns
	) {
		switch( iterator->status ) {
			case HEXD_DUPLICATE:
			case HEXD_DUPLICATE_MORE:
				iterator->status = HEXD_DUPLICATE_MORE;
				break;
			case HEXD_LAST_LINE:
				XLOG_ASSERT( 0 );
			default:
				iterator->status = HEXD_DUPLICATE;
				break;
		}
		
		return n;
	}
	if( n < options->columns || ( options->end != -1 && options->end <= iterator->offset ) ) {
		iterator->status = HEXD_LAST_LINE;
	} else {
		iterator->status = HEXD_NORMAL;
	}
	
	// Print hex area
	const char *prev_fmt = NULL;
	for( size_t j = 0; j < options->columns; j ++ ) {
		if( options->groupsize != 0 && j % options->groupsize == 0 && j != 0 ) {
			if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
				*dumpptr++ = ' ';
			}
		}
		
		if( j < n ) {
			const char *fmt = hexdump_format_of( iterator->line[j] );
			if( prev_fmt != fmt && options->use_formatting ) {
				if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
					dumpptr += snprintf(
						dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
						"\x1B[%sm", fmt
					);
				}
			}
			if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
				dumpptr += snprintf(
					dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
					"%s%02x", j == 0 ? "" : " ", iterator->line[j]
				);
			}
			prev_fmt = fmt;
		} else {
			if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
				dumpptr += snprintf(
					dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
					"%s", "   "
				);
			}
		}
	}
	if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
		*dumpptr++ = ' ';
	}
	
	// Print char area
	for( size_t j = 0; j < options->columns; j ++ ) {
		if( options->groupsize != 0 && j % options->groupsize == 0 ) {
			if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
				*dumpptr++ = ' ';
			}
		}
		
		if( j < n ) {
			const char *fmt = hexdump_format_of( iterator->line[j] );
			if( prev_fmt != fmt && options->use_formatting ) {
				if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
					dumpptr += snprintf(
						dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
						"\x1B[%sm", fmt
					);
				}
			}
			if( iterator->line[j] >= 0x80 ) {
				if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
					dumpptr += snprintf(
						dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
						"%s", CHAR_AREA_HIGH_LUT[iterator->line[j] - 0x80]
					);
				}
			} else {
				if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
					*dumpptr++ = isprint( iterator->line[j] ) ? iterator->line[j] : '.';
				}
			}
			prev_fmt = fmt;
		} else {
			if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
				*dumpptr++ = ' ';
			}
		}
	}
	if( ( dumpptr - iterator->dumpline ) < iterator->dumpsize ) {
		dumpptr += snprintf(
			dumpptr, iterator->dumpsize - ( dumpptr - iterator->dumpline ),
			"%s", options->use_formatting ? "\x1B[m" : ""
		);
	}
	memcpy( iterator->prev_line, iterator->line, options->columns );
	
	return n;
}

int __hexdump(
    const void *target, const hexdump_options_t *options,
    int ( *readline )( const void * /* target */, off_t /* offset */, void * /* buffer */, size_t /* size */ ),
    void ( *printline )( uintmax_t, const char *, void * ), void *arg
)
{
	XLOG_ASSERT( target );
	XLOG_ASSERT( options );
	XLOG_ASSERT( readline );
	
	hexdump_iterator_t iterator = {
		.offset = options->start,
		.dumpline = NULL,
		.dumpsize = 0,
		.status = HEXD_FIRST_LINE,
		.line = NULL,
		.prev_line = NULL,
	};
	
	size_t dumpline_size = 0;
	char *dumpline = NULL;
	
	unsigned char *line = ( unsigned char * )calloc( 1, options->columns );
	if( NULL == line ) {
		return -1;
	}
	unsigned char *prev_line = ( unsigned char * )calloc( 1, options->columns );
	if( NULL == prev_line ) {
		free( line );
		return -1;
	}
	
	///< xxxxxyyy  hh hh hh ... hh  ee ee ee ... ee  abcdefgh abcdefgh
	dumpline_size = (
        ( 5 + 3 )/* offset */ + 2/* space */ + 1 /*  */
        + ( options->columns / options->groupsize )/* space */
        + ( 2/* hex */ + 1/* space */ + HEXD_LINE_RSV_SIZE_COLORS ) * options->columns
    );
	dumpline = ( char * )malloc( dumpline_size );
	if( NULL == dumpline ) {
		free( line );
		free( prev_line );
		return -1;
	}
	
	iterator.dumpline = dumpline;
	iterator.dumpsize = dumpline_size;
	iterator.line = line;
	iterator.prev_line = prev_line;
	
	while( true ) {
		int n = __hexdump_iterator( &iterator, target, options, readline );
		uintmax_t cursor = iterator.offset - n;
		if( iterator.status == HEXD_EOF ) {
			break;
		} else if( iterator.status == HEXD_DUPLICATE ) {
			snprintf( dumpline, dumpline_size, "\x0D%8s", "*" );
			if( printline ) {
				printline( cursor, dumpline, arg );
			} else {
				printf( "%s\n", dumpline );
			}
			continue;
		} else if( iterator.status == HEXD_DUPLICATE_MORE ) {
			continue;
		} else {
			if( printline ) {
				printline( cursor, dumpline, arg );
			} else {
				printf( "%5jx%03jx  %s\n", cursor >> 12, cursor & 0xFFF, dumpline );
			}
			if( iterator.status == HEXD_LAST_LINE ) {
				break;
			}
		}
	}
	free( line );
	free( prev_line );
	free( dumpline );
	
	return 0;
}

#if 0
static void hexdump_printline( uintmax_t cursor, const char *dumpline, void *arg )
{
	( void )arg;
	printf( "%5jx%03jx  %s\n", cursor >> 12, cursor & 0xFFF, dumpline );
}

static int hexdump_memory_readline( const void *addr, off_t offset, void *buffer, size_t size )
{
	memcpy( buffer, ( const char * )addr + offset, size );
	
	return ( int )size;
}

void hexdump_memory( const void *addr, size_t size, hexdump_options_t *options )
{
	hexdump_options_t hexdopt_def = {
		.start = 0,
		.end = -1,
		.columns = 16,
		.groupsize = 8,
		.use_formatting = false,
	};
	hexdump_options_t *op = options ? options : &hexdopt_def;
	
	if( -1 == op->end ) {
		op->end = op->start + ( int )size;
	}
	
	__hexdump( addr, op, hexdump_memory_readline, hexdump_printline, NULL );
}

static int hexdump_file_readline( const void *target, off_t offset, void *buffer, size_t size )
{
	int fd = *( const int * )target;
	
	off_t cur = lseek( fd, offset, SEEK_SET );
	if( cur < 0 ) {
		return -1;
	}
	return ( int )read( fd, buffer, size );
}

void hexdump_file( const char *file, const hexdump_options_t *options )
{
	XLOG_ASSERT( options );
	
	int fd = open( file, O_RDONLY );
	if( fd < 0 ) {
		warn( "failed to open %s.", file );
		return;
	}
	
	__hexdump( &fd, options, hexdump_file_readline, NULL, NULL );
}

/**
 * Parses a range "start-end" (both ends optional)
 * or "start+size" (neither optional) into a `hexdump_options_t` instance.
 */
int hexdump_parse_range( const char *str, hexdump_options_t *options )
{
	XLOG_ASSERT( str );
	XLOG_ASSERT( options );
	
	const char *first = str, *delim = str + strcspn( str, "+-" ), *second = delim + 1;
	
	if( *delim == '\0' ) {
		errx( 1, "no delimiter in range %s", str );
		return -1;
	}
	
	char *end;
	if( first != delim ) {
		options->start = strtoimax( first, &end, 0 );
		if( !isdigit( *first ) || end != delim ) {
			errx( 1, "invalid start value in range %s", str );
			return -1;
		}
	}
	if( *second != '\0' ) {
		options->end = strtoimax( second, &end, 0 );
		if( !isdigit( *second ) || *end != '\0' ) {
			errx( 1, "invalid end/size value in range %s", str );
			return -1;
		}
	}
	
	if( *delim == '+' ) {
		if( first == delim ) {
			errx( 1, "start unspecified in range %s", str );
			return -1;
		}
		if( *second == '\0' ) {
			errx( 1, "size unspecified in range %s", str );
			return -1;
		}
		options->end += options->start;
	}
	
	if( options->end < options->start && options->end != -1 ) {
		errx( 1, "end was less than start in range %s", str );
		return -1;
	}
	
	return 0;
}

int hexdump_shell_main( int argc, char **argv )
{
	hexdump_options_t hexdopt  = {
		.start = 0,
		.end = -1,
		.columns = 16,
		.groupsize = 8,
		.use_formatting = true,
	};
	// Default to colourful output if output is a TTY
	hexdopt.use_formatting = isatty( 1 );
	// Parse options
	int opt;
	while( ( opt = getopt( argc, argv, "g:pPr:w:" ) ) != -1 ) {
		switch ( opt ) {
			case 'g':
				hexdopt.groupsize = ( size_t )atol( optarg );
				break;
			case 'p':
				hexdopt.use_formatting = false;
				break;
			case 'P':
				hexdopt.use_formatting = true;
				break;
			case 'r':
				hexdump_parse_range( optarg, &hexdopt );
				break;
			case 'w':
				hexdopt.columns = ( size_t )atol( optarg );
				break;
			default:
				fprintf( stderr, "usage: hexd [-p] [-P] [-g groupsize] [-r range] [-w width]\n" );
				return 1;
		}
	}
	argc -= optind;
	argv += optind;
	
	// Hexdump files
	if( argc > 0 ) {
		for ( int i = 0; i < argc; i++ ) {
			if ( argc > 1 ) {
				printf(
				    "%s====> hexdump of file %s%s%s <====\n", i > 0 ? "\n" : "",
				    hexdopt.use_formatting ? "\x1B[1m" : "",
				    argv[i],
				    hexdopt.use_formatting ? "\x1B[m" : ""
				);
			}
			
			hexdump_file( argv[i], &hexdopt );
		}
	}
	
	return 0;
}
#endif
