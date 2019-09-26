#ifndef __HEXDUMP_H
#define __HEXDUMP_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

typedef struct {
	off_t start, end;
	size_t columns, groupsize;
	bool use_formatting;
} hexdump_options_t;

typedef enum {
	HEXD_EOF = -1,
	HEXD_NORMAL = 0,
	HEXD_FIRST_LINE,
	HEXD_LAST_LINE,
	HEXD_DUPLICATE,
	HEXD_DUPLICATE_MORE,
} hexdump_status_t;

typedef struct {
	off_t offset;
	char *dumpline;
	size_t dumpsize;
	hexdump_status_t status;
	
	unsigned char *line;
	unsigned char *prev_line;
} hexdump_iterator_t;

int __hexdump_iterator(
	hexdump_iterator_t *iterator,
	const void *target, const hexdump_options_t *options,
	int ( *readline )( const void * /* target */, off_t /* offset */, void * /* buffer */, size_t /* size */ )
);

int __hexdump(
	const void *target, const hexdump_options_t *options,
	int ( *readline )( const void * /* target */, off_t /* offset */, void * /* buffer */, size_t /* size */ ),
	void ( *printline )( uintmax_t, const char *, void * ), void *arg
);

#if 0
void hexdump_memory( const void *addr, size_t size, hexdump_options_t *options );

void hexdump_file( const char *file, const hexdump_options_t *options );

/**
 * Parses a range "start-end" (both ends optional)
 * or "start+size" (neither optional) into a `hexdump_options_t` instance.
 */
int hexdump_parse_range( const char *str, hexdump_options_t *options );

int hexdump_shell_main( int argc, char **argv );
#endif

#endif
