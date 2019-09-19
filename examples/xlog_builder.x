#include <xlog/xlog.h>
#include <xlog/xlog_tree.h>
#include <xlog/xlog_payload.h>
#include <xlog/xlog_hexdump.h>

#include "internal/xlog.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wembedded-directive"
#endif


/** extended logging builder with xlog-payload */

/** create a payload object(TIME) */
static xlog_payload_t *xlog_payload_create_LOG_TIME( void )
{
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

	xlog_payload_t *payload = xlog_payload_create(XLOG_PAYLOAD_ID_LOG_TIME, "Time", XLOG_PAYLOAD_OBINARY | XLOG_PAYLOAD_OALLOC_ONCE, sizeof( xlog_time_t ));
	if( payload ) {
		xlog_payload_append_binary( &payload, &now, sizeof( xlog_time_t ) );
	}
	return payload;
}

/** create a payload object(LOG_CLASS) */
static xlog_payload_t *xlog_payload_create_LOG_CLASS( const char *level, const char *module )
{
	XLOG_ASSERT( level );
	xlog_payload_t *payload = xlog_payload_create(
		XLOG_PAYLOAD_ID_LOG_CLASS, "Log Class",
		XLOG_PAYLOAD_OTEXT | XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN,
		XLOG_PAYLOAD_TEXT_LENGTH( level ) + ( module ? XLOG_PAYLOAD_TEXT_LENGTH( module ) : 0 ) + 15, 16
	);
	if( payload ) {
		xlog_payload_append_text( &payload, XLOG_TAG_PREFIX_LOG_CLASS );
		xlog_payload_append_text( &payload, level );
		if( module ) {
			xlog_payload_append_text( &payload, module );
		}
		xlog_payload_append_text( &payload, XLOG_TAG_SUFFIX_LOG_CLASS );
	}
	return payload;
}

/** create a payload object(LOC) */
static xlog_payload_t *xlog_payload_create_LOG_POINT( const char *file, const char *func, int line )
{
	xlog_payload_t *payload = xlog_payload_create( XLOG_PAYLOAD_ID_LOG_POINT, "Source Location", XLOG_PAYLOAD_OTEXT | XLOG_PAYLOAD_ODYNAMIC, 64 );
	if( payload ) {
		xlog_payload_append_text( &payload, XLOG_TAG_PREFIX_LOG_POINT );
		if( file ) {
			xlog_payload_append_text( &payload, file );
		}
		if( func ) {
			if( file ) {
				xlog_payload_append_text( &payload, " " );
			}
			xlog_payload_append_text( &payload, func );
		}
		if( line != -1 ) {
			if( file || func ) {
				xlog_payload_append_text( &payload, ":" );
			}
			char buff[12];
			snprintf( buff, sizeof( buff ), "%d", line );
			xlog_payload_append_text( &payload, buff );
		}
		xlog_payload_append_text( &payload, XLOG_TAG_SUFFIX_LOG_POINT );
	}
	return payload;
}

/** create a payload object(LOG_TASK) */
static xlog_payload_t *xlog_payload_create_LOG_TASK( void )
{
	xlog_payload_t *payload = xlog_payload_create(XLOG_PAYLOAD_ID_LOG_TASK, "Task Info", XLOG_PAYLOAD_OTEXT | XLOG_PAYLOAD_ODYNAMIC, 32);
	if( payload ) {
		char taskname[XLOG_LIMIT_THREAD_NAME];
		XLOG_GET_THREAD_NAME( taskname );
		xlog_payload_append_text_va(
			&payload, XLOG_TAG_PREFIX_LOG_TASK "%d/%d %s" XLOG_TAG_SUFFIX_LOG_TASK,
			getppid(), getpid(), taskname
		);
	}
	return payload;
}

static int xlog_payload_print_LOG_TIME(
    const xlog_payload_t *payload, xlog_printer_t *printer
)
{
	XLOG_ASSERT( payload );
	XLOG_ASSERT( XLOG_PAYLOAD_ID_LOG_TIME == payload->id );
	XLOG_ASSERT( sizeof( xlog_time_t ) == payload->length );
	
	xlog_time_t *xtm = ( xlog_time_t * )xlog_payload_data_vptr( payload );
	time_t tv_sec = xtm->tv.tv_sec;
	struct tm *p = localtime( &tv_sec );
	
	char buffer[48];
	snprintf(
        buffer, sizeof(buffer),
        XLOG_TAG_PREFIX_LOG_TIME "%02d/%02d %02d:%02d:%02d.%03d" XLOG_TAG_SUFFIX_LOG_TIME,
        p->tm_mon + 1, p->tm_mday,
        p->tm_hour, p->tm_min, p->tm_sec, ( int )( ( xtm->tv.tv_usec + 500 ) / 1000 )
    );
    
	return printer->append( printer, buffer );
}

static const struct {
	int id;
	int (*print)( const xlog_payload_t *, xlog_printer_t * );
} payload_print_funcs[] = {
	{ XLOG_PAYLOAD_ID_AUTO		, NULL },
	{ XLOG_PAYLOAD_ID_TEXT		, xlog_payload_print_TEXT },
	{ XLOG_PAYLOAD_ID_BINARY	, xlog_payload_print_BINARY },
	{ XLOG_PAYLOAD_ID_LOG_TIME	, xlog_payload_print_LOG_TIME },
	{ XLOG_PAYLOAD_ID_LOG_CLASS	, xlog_payload_print_TEXT },
	{ XLOG_PAYLOAD_ID_LOG_POINT	, xlog_payload_print_TEXT },
	{ XLOG_PAYLOAD_ID_LOG_TASK	, xlog_payload_print_TEXT },
	{ XLOG_PAYLOAD_ID_LOG_BODY	, xlog_payload_print_TEXT },
};

static int xlog_payload_print(
    const xlog_payload_t *payload, xlog_printer_t *printer
)
{
	XLOG_ASSERT( payload );
	XLOG_ASSERT( payload->id >= XLOG_PAYLOAD_ID_AUTO && payload->id <= XLOG_PAYLOAD_ID_LOG_BODY );
	
	int (*print)( const xlog_payload_t *, xlog_printer_t * ) = payload_print_funcs[payload->id].print;
	if(print) {
		return print( payload, printer );
	}
	return 0;
}

struct xlog_list_tag {
	xlog_payload_t *payload;
	struct xlog_list_tag *next, *prev;
};
#include "internal/xlog_list.h"

static void *xlog_payload_list_create( void )
{
	xlog_list_t *context = ( xlog_list_t * )XLOG_MALLOC( sizeof( xlog_list_t ) );
	
	if( context ) {
		xlog_list_init( context );
	}
	
	return context;
}

static int xlog_payload_list_append( void *context, const xlog_payload_t *payload )
{
	xlog_list_t *head = ( xlog_list_t * )context;
	xlog_list_t *entry = (xlog_list_t *)XLOG_MALLOC( sizeof( xlog_list_t ) );
	if( entry ) {
		xlog_list_init( entry );
		entry->payload = ( xlog_payload_t * )payload;
		xlog_list_add( entry, head );
	}
	
	return 0;
}

static void xlog_payload_list_destory( void *context )
{
	xlog_list_t *head = ( xlog_list_t * )context;
	xlog_list_t *cur = head->next, *aux = NULL;
	
	while( cur != head ) {
		aux = cur;
		xlog_payload_destory( &(aux->payload) );
		cur = aux->next;
		XLOG_FREE( aux );
	}
	XLOG_FREE( head );
}

static int xlog_payload_list_print( void *context, xlog_printer_t *printer )
{
	int length = 0;
	xlog_list_t *head = ( xlog_list_t * )context;
	xlog_list_t *pos = NULL;
	printer->control( printer, XLOG_PRINTER_CTRL_LOCK, NULL );
	xlog_list_for_each_prev( pos, head ) {
		length += xlog_payload_print(
			pos->payload, printer
		);
	}
	printer->control( printer, XLOG_PRINTER_CTRL_UNLOCK, NULL );
	return length;
}

static inline void *xlog_build_log_context_va(
    xlog_t *context, xlog_module_t *module, int level,
    const char *file, const char *func, int line,
    const char *format, va_list args
)
{
	XLOG_ASSERT( context );
	XLOG_ASSERT( module );

	void *list = xlog_payload_list_create();
	if( NULL == list ) {
		XLOG_TRACE( "failed to create payload-list." );
		return NULL;
	}
	
	/** global setting in xlog */
	if(
		module && context
	    && ( !(context->options & XLOG_CONTEXT_OALIVE))
	) {
		XLOG_TRACE( "drop by context." );
		return NULL;
	}
	
	/** module based limit */
	if( module && XLOG_IF_DROP_LEVEL( level, module->level ) ) {
		XLOG_TRACE( "drop by module." );
		return NULL;
	}
	
	xlog_payload_t *payload = NULL;
	
	/* package time */
	if( xlog_if_format_enabled( module, level, XLOG_FORMAT_OTIME ) ) {
		payload = xlog_payload_create_LOG_TIME();
		xlog_payload_list_append( list, payload );
	}
	
	/* package task info */
	if( xlog_if_format_enabled( module, level, XLOG_FORMAT_OTASK ) ) {
		payload = xlog_payload_create_LOG_TASK();
		xlog_payload_list_append( list, payload );
	}
	
	/* package level and module info */
	if( module && xlog_if_format_enabled( module, level, XLOG_FORMAT_OLEVEL | XLOG_FORMAT_OMODULE ) ) {
		char modulename[XLOG_LIMIT_MODULE_PATH] = { 0 };
		const char *modulepath = NULL;
		if( xlog_if_format_enabled( module, level, XLOG_FORMAT_OMODULE ) ) {
			modulepath = xlog_module_name( modulename, XLOG_LIMIT_MODULE_PATH, module );
		}

		payload = xlog_payload_create_LOG_CLASS( context->attributes[level].class_prefix, modulepath );
		xlog_payload_list_append( list, payload );
	}
	
	/* package file directory and name, function name and line number info */
	if( xlog_if_format_enabled( module, level, XLOG_FORMAT_OLOCATION ) ) {
		payload = xlog_payload_create_LOG_POINT(
			xlog_if_format_enabled( module, level, XLOG_FORMAT_OFILE ) ? file : NULL,
			xlog_if_format_enabled( module, level, XLOG_FORMAT_OFUNC ) ? func : NULL,
			xlog_if_format_enabled( module, level, XLOG_FORMAT_OLINE ) ? line : -1
		);
		xlog_payload_list_append( list, payload );
	}
	
	/* args point to the first variable parameter */
	payload = xlog_payload_create( XLOG_PAYLOAD_ID_LOG_BODY, "Log Text", XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN, 240, 32 );
	if( context && ( context->options & XLOG_CONTEXT_OCOLOR_BODY ) ) {
		xlog_payload_append_text( &payload, context->attributes[level].body_prefix );
	}
	xlog_payload_append_text_va_list( &payload, format, args );
	if( context && ( context->options & XLOG_CONTEXT_OCOLOR_BODY ) ) {
		xlog_payload_append_text( &payload, context->attributes[level].body_suffix );
	}
	xlog_payload_append_text( &payload, XLOG_STYLE_NEWLINE );
	xlog_payload_list_append( list, payload );
	
	return list;
}

void *xlog_build_log_context(
    xlog_t *context, xlog_module_t *module, int level,
    const char *file, const char *func, long int line,
    const char *format, ...
)
{
	void *list = NULL;
	va_list ap;
	va_start( ap, format );
	list = xlog_build_log_context_va( context, module, level, file, func, line, format, ap );
	va_end( ap );
	
	return list;
}

int xlog_output(
	xlog_printer_t *printer,
    xlog_t *context, xlog_module_t *module, int level,
    const char *file, const char *func, long line,
    const char *format, ...
)
{
	int length = 0;
	void *list = NULL;
	va_list ap;
	va_start( ap, format );
	list = xlog_build_log_context_va( context, module, level, file, func, line, format, ap );
	va_end( ap );
	
	if( list ) {
		length = xlog_payload_list_print( list, printer );
		xlog_payload_list_destory( list );
	}
	
	return length;
}


void *
xlog_build_log_context(
    xlog_t *master, xlog_module_t *module, int level,
    const char *file, const char *func, long int line,
    const char *format, ...
);

int xlog_output(
	xlog_printer_t *printer,
    xlog_t *master, xlog_module_t *module, int level,
    const char *file, const char *func, long line,
    const char *format, ...
);