#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include <xlog/plugins/hexdump.h>
#include <xlog/plugins/ringbuf.h>
#include <xlog/plugins/family_tree.h>

static int random_integer( void )
{
	static bool initialized = false;
	if( !initialized ) {
		unsigned int seed;
		
		int fd = open( "/dev/urandom", O_RDONLY );
		if( fd < 0 || read( fd, &seed, sizeof( seed ) ) < 0 ) {
			seed = time( NULL );
		}
		if( fd >= 0 ) {
			close( fd );
			fd = -1;
		}
		
		srand( seed );
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

int main( int argc, char **argv )
{
	(void)argc;
	(void)argv;
	
	// hexdump-memory
	{
		int buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer();
		}
		hexdump_memory( buff, sizeof( buff ), NULL );
		
		memset( buff + 33 + 1024, 2, 32 * 4 );
		hexdump_memory( buff, sizeof( buff ), NULL );
	}
	
	// hexdump-file and shell
	{
		int buff[2048];
		for( int i = 0; i < sizeof( buff ) / sizeof( *buff ); i ++ ) {
			buff[i] = random_integer();
		}
		int fd = open( "hexdump-file.test", O_WRONLY | O_CREAT, 0644 );
		write( fd, buff, sizeof( buff ) );
		close( fd );
		
		hexdump_file( "hexdump-file.test", NULL );
		
		char cmdline[] = "hexd -P -g 8 -r 128+56 -w 16 hexdump-file.test hexdump-file.test";
		int _argc;
		char *_argv[20];
		shell_make_args( cmdline, &_argc, _argv, 19 );
		hexdump_shell_main( _argc, _argv );
	}
	
	return 0;
}