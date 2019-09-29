#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "internal.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

/** STATIC printer: stdout(default)/stderr */
static struct __stdio_printer_context {
    pthread_mutex_t lock;
    #if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_PRINTER)
    xlog_stats_t stats;
    #endif
}
stdout_printer_context = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
},
stderr_printer_context = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
};

static int __stdxxx_append( xlog_printer_t *printer, void *data )
{
    const char *text = ( const char * )data;
    #ifndef XLOG_BENCH_NO_OUTPUT
    return fprintf(
        XLOG_PRINTER_TYPE_GET(printer->options) == XLOG_PRINTER_STDOUT ? stdout : stderr,
        "%s", text
    );
    #else
    return strlen( text );
    #endif
}

static int __stdxxx_optctl( xlog_printer_t *printer, int option, void *vptr, size_t size )
{
    struct __stdio_printer_context *context = ( struct __stdio_printer_context * )printer->context;
    switch( option ) {
        case XLOG_PRINTER_CTRL_LOCK: {
            pthread_mutex_lock( &context->lock );
        } break;
        case XLOG_PRINTER_CTRL_UNLOCK: {
            pthread_mutex_unlock( &context->lock );
        } break;
        case XLOG_PRINTER_CTRL_GABICLR: {
            if( size == sizeof( int ) && vptr ) {
                *((int *)vptr) = 1;
            }
        } break;
        default: {
            return -1;
        }
    }
    return 0;
}

xlog_printer_t stdout_printer = {
    #if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
    .magic = XLOG_MAGIC_PRINTER,
    #endif
    .context = ( void * ) &stdout_printer_context,
    .options = XLOG_PRINTER_STDOUT,
    .append = __stdxxx_append,
    .optctl = __stdxxx_optctl,
},
stderr_printer = {
    #if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
    .magic = XLOG_MAGIC_PRINTER,
    #endif
    .context = ( void * ) &stderr_printer_context,
    .options = XLOG_PRINTER_STDERR,
    .append  = __stdxxx_append,
    .optctl = __stdxxx_optctl,
};
