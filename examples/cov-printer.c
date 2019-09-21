#define XLOG_PRINTER	g_printer

#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

static xlog_printer_t *g_printer = NULL;
static xlog_module_t *g_mod = NULL;
#define XLOG_MODULE g_mod

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	
	g_mod = xlog_module_open("/net", XLOG_LEVEL_DEBUG, NULL);
	
	g_printer = xlog_printer_create( XLOG_PRINTER_STDOUT );
	for(int i = 0; i < 10; i ++) {
		log_r( "test info: upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf\n" );
	}
	xlog_printer_destory( g_printer );
	
	g_printer = xlog_printer_create( XLOG_PRINTER_STDERR );
	for(int i = 0; i < 10; i ++) {
		log_r( "test info: upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf\n" );
	}
	xlog_printer_destory( g_printer );
	
	g_printer = xlog_printer_create( XLOG_PRINTER_FILES_ROTATING, "rotating.txt", 1024 * 8, 16 );
	for(int i = 0; i < 10; i ++) {
		log_w( "test info: upoggjqjaxtmvejcbdyiluqzcogqxbftwuzqwelfywgwmxwghezcwgxlbbyrrmf\n" );
	}
	xlog_printer_destory( g_printer );
	
	return 0;
}
