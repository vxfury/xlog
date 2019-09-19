#include <stdlib.h>

#include <xlog/xlog_config.h>
#include "internal/xlog.h"

#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
/* allocate memory and set magic for it */
void *xlog_malloc( int magic, size_t nbytes )
{
	void *vptr = malloc( sizeof( int ) + nbytes );
	if( vptr ) {
		*(int *)vptr = magic;
		vptr = (void *)((int *)vptr + 1);
	}
	
	return vptr;
}

/* clear magic of memory and free it */
void xlog_free( void *vptr )
{
	if( vptr ) {
		vptr = (void *)((int *)vptr - 1);
		*(int *)vptr = 0;
		free( vptr );
	}
}

/* resize memory allocated, no changes on magic */
void *xlog_realloc( void *vptr, size_t nbytes )
{
	if( vptr ) {
		vptr = (void *)((int *)vptr - 1);
		return realloc( vptr, sizeof( int ) + nbytes );
	}
	return NULL;
}
#endif
