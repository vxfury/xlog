#include "plugins/getopt.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

/* macros */

/* types */
typedef enum GETOPT_ORDERING_T {
	PERMUTE,
	RETURN_IN_ORDER,
	REQUIRE_ORDER
} GETOPT_ORDERING_T;

/* functions */

/* reverse_argv_elements:  reverses num elements starting at argv */
static void reverse_argv_elements( char **argv, int num )
{
	int i;
	char *tmp;
	
	for(
		i = 0; i < ( num >> 1 ); i++
	) {
		tmp = argv[i];
		argv[i] = argv[num - i - 1];
		argv[num - i - 1] = tmp;
	}
}

/* permute: swap two blocks of argv-elements given their lengths */
static void permute( char *const argv[], int len1, int len2 )
{
	reverse_argv_elements( ( char ** )argv, len1 );
	reverse_argv_elements( ( char ** )argv, len1 + len2 );
	reverse_argv_elements( ( char ** )argv, len2 );
}

/* is_option: is this argv-element an option or the end of the option list? */
static int is_option( char *argv_element, int only )
{
	return (
			( argv_element == 0 )
			|| ( argv_element[0] == '-' ) || ( only && argv_element[0] == '+' )
		);
}

/* getopt_internal:  the function that does all the dirty work
   NOTE: to reduce the code and RAM footprint this function uses
   fputs()/fputc() to do output to stderr instead of fprintf(). */
static int getopt_internal(
	int argc, char *const argv[], const char *shortopts,
	const struct option *longopts, int *longind, int only,
	struct getopt_data *data
)
{
	GETOPT_ORDERING_T ordering = PERMUTE;
	size_t permute_from = 0;
	int num_nonopts = 0;
	int optindex = 0;
	size_t match_chars = 0;
	char *possible_arg = 0;
	int longopt_match = -1;
	int has_arg = -1;
	char *cp = 0;
	int arg_next = 0;
	int initial_colon = 0;
	
	/* first, deal with silly parameters and easy stuff */
	if( argc == 0 || argv == 0 || ( shortopts == 0 && longopts == 0 )
		|| data->optind >= argc || argv[data->optind] == 0 ) {
		return EOF;
	}
	if( strcmp( argv[data->optind], "--" ) == 0 ) {
		data->optind++;
		return EOF;
	}
	
	/* if this is our first time through */
	if( data->optind == 0 ) {
		data->optind = data->optwhere = 1;
	}
	
	/* define ordering */
	if( shortopts != 0 && ( *shortopts == '-' || *shortopts == '+' ) ) {
		ordering = ( *shortopts == '-' ) ? RETURN_IN_ORDER : REQUIRE_ORDER;
		shortopts++;
	} else {
		ordering = ( getenv( "POSIXLY_CORRECT" ) != 0 ) ? REQUIRE_ORDER : PERMUTE;
	}
	
	/* check for initial colon in shortopts */
	if( shortopts != 0 && *shortopts == ':' ) {
		++shortopts;
		initial_colon = 1;
	}
	
	/*
	 * based on ordering, find our next option, if we're at the beginning of
	 * one
	 */
	if( data->optwhere == 1 ) {
		switch( ordering ) {
			default:        /* shouldn't happen */
			case PERMUTE:
				permute_from = data->optind;
				num_nonopts = 0;
				while( !is_option( argv[data->optind], only ) ) {
					data->optind++;
					num_nonopts++;
				}
				if( argv[data->optind] == 0 ) {
					/* no more options */
					data->optind = permute_from;
					return EOF;
				} else if( strcmp( argv[data->optind], "--" ) == 0 ) {
					/* no more options, but have to get `--' out of the way */
					permute( argv + permute_from, num_nonopts, 1 );
					data->optind = permute_from + 1;
					return EOF;
				}
				break;
			case RETURN_IN_ORDER:
				if( !is_option( argv[data->optind], only ) ) {
					data->optarg = argv[data->optind++];
					return ( data->optopt = 1 );
				}
				break;
			case REQUIRE_ORDER:
				if( !is_option( argv[data->optind], only ) ) {
					return EOF;
				}
				break;
		}
	}
	/* End of option list? */
	if( argv[data->optind] == 0 ) {
		return EOF;
	}
	
	/* we've got an option, so parse it */
	
	/* first, is it a long option? */
	if( longopts != 0
		&& (
			memcmp( argv[data->optind], "--", 2 ) == 0
			|| ( only && argv[data->optind][0] == '+' )
		) && data->optwhere == 1 ) {
		/* handle long options */
		if( memcmp( argv[data->optind], "--", 2 ) == 0 ) {
			data->optwhere = 2;
		}
		longopt_match = -1;
		possible_arg = strchr( argv[data->optind] + data->optwhere, '=' );
		if( possible_arg == 0 ) {
			/* no =, so next argv might be arg */
			match_chars = strlen( argv[data->optind] );
			possible_arg = argv[data->optind] + match_chars;
			match_chars = match_chars - data->optwhere;
		} else {
			match_chars = ( possible_arg - argv[data->optind] ) - data->optwhere;
		}
		for(
			optindex = 0; longopts[optindex].name != 0; ++optindex
		) {
			if( memcmp
				(
					argv[data->optind] + data->optwhere, longopts[optindex].name,
					match_chars
				) == 0 ) {
				/* do we have an exact match? */
				if( match_chars == strlen( longopts[optindex].name ) ) {
					longopt_match = optindex;
					break;
				}
				/* do any characters match? */
				else {
					if( longopt_match < 0 ) {
						longopt_match = optindex;
					} else {
						/* we have ambiguous options */
						if( data->opterr ) {
							fputs( argv[0], stderr );
							fputs( ": option `", stderr );
							fputs( argv[data->optind], stderr );
							fputs( "' is ambiguous (could be `--", stderr );
							fputs( longopts[longopt_match].name, stderr );
							fputs( "' or `--", stderr );
							fputs( longopts[optindex].name, stderr );
							fputs( "')\n", stderr );
						}
						return ( data->optopt = '?' );
					}
				}
			}
		}
		if( longopt_match >= 0 ) {
			has_arg = longopts[longopt_match].has_arg;
		}
	}
	
	/* if we didn't find a long option, is it a short option? */
	if( longopt_match < 0 && shortopts != 0 ) {
		cp = strchr( shortopts, argv[data->optind][data->optwhere] );
		if( cp == 0 ) {
			/* couldn't find option in shortopts */
			if( data->opterr ) {
				fputs( argv[0], stderr );
				fputs( ": invalid option -- `-", stderr );
				fputc( argv[data->optind][data->optwhere], stderr );
				fputs( "'\n", stderr );
			}
			data->optwhere++;
			if( argv[data->optind][data->optwhere] == '\0' ) {
				data->optind++;
				data->optwhere = 1;
			}
			return ( data->optopt = '?' );
		}
		has_arg = (
				( cp[1] == ':' )
				? ( ( cp[2] == ':' ) ? OPTIONAL_ARG : REQUIRED_ARG ) : NO_ARG
			);
		possible_arg = argv[data->optind] + data->optwhere + 1;
		data->optopt = *cp;
	}
	
	/* get argument and reset data->optwhere */
	arg_next = 0;
	switch( has_arg ) {
		case OPTIONAL_ARG:
			if( *possible_arg == '=' ) {
				possible_arg++;
			}
			data->optarg = ( *possible_arg != '\0' ) ? possible_arg : 0;
			data->optwhere = 1;
			break;
		case REQUIRED_ARG:
			if( *possible_arg == '=' ) {
				possible_arg++;
			}
			if( *possible_arg != '\0' ) {
				data->optarg = possible_arg;
				data->optwhere = 1;
			} else if( data->optind + 1 >= argc ) {
				if( data->opterr ) {
					fputs( argv[0], stderr );
					fputs( ": argument required for option `-", stderr );
					if( longopt_match >= 0 ) {
						fputc( '-', stderr );
						fputs( longopts[longopt_match].name, stderr );
						data->optopt = initial_colon ? ':' : '\?';
					} else {
						fputc( *cp, stderr );
						data->optopt = *cp;
					}
					fputs( "'\n", stderr );
				}
				data->optind++;
				return initial_colon ? ':' : '\?';
			} else {
				data->optarg = argv[data->optind + 1];
				arg_next = 1;
				data->optwhere = 1;
			}
			break;
		default:            /* shouldn't happen */
		case NO_ARG:
			if( longopt_match < 0 ) {
				data->optwhere++;
				if( argv[data->optind][data->optwhere] == '\0' ) {
					data->optwhere = 1;
				}
			} else {
				data->optwhere = 1;
			}
			data->optarg = 0;
			break;
	}
	
	/* do we have to permute or otherwise modify data->optind? */
	if( ordering == PERMUTE && data->optwhere == 1 && num_nonopts != 0 ) {
		permute( argv + permute_from, num_nonopts, 1 + arg_next );
		data->optind = permute_from + 1 + arg_next;
	} else if( data->optwhere == 1 ) {
		data->optind = data->optind + 1 + arg_next;
	}
	
	/* finally return */
	if( longopt_match >= 0 ) {
		if( longind != 0 ) {
			*longind = longopt_match;
		}
		if( longopts[longopt_match].flag != 0 ) {
			*( longopts[longopt_match].flag ) = longopts[longopt_match].val;
			return 0;
		} else {
			return longopts[longopt_match].val;
		}
	} else {
		return data->optopt;
	}
}

int getopt_r(
	int argc, char *const argv[], const char *optstring,
	struct getopt_data *data
)
{
	return getopt_internal( argc, argv, optstring, 0, 0, 0, data );
}

int getopt_long_r(
	int argc, char *const argv[], const char *shortopts,
	const struct option *longopts, int *longind,
	struct getopt_data *data
)
{
	return getopt_internal( argc, argv, shortopts, longopts, longind, 0, data );
}

int getopt_long_only_r(
	int argc, char *const argv[], const char *shortopts,
	const struct option *longopts, int *longind,
	struct getopt_data *data
)
{
	return getopt_internal( argc, argv, shortopts, longopts, longind, 1, data );
}
