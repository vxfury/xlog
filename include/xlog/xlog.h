#ifndef __XLOG_H
#define __XLOG_H

#include <xlog/xlog_config.h>
#include <xlog/xlog_helper.h>

#ifdef __cplusplus
extern "C" {
#endif

/** xlog level */
#define XLOG_LEVEL_SILENT			__XLOG_LEVEL_SILENT
#define XLOG_LEVEL_FATAL 			__XLOG_LEVEL_FATAL
#define XLOG_LEVEL_ERROR 			__XLOG_LEVEL_ERROR
#define XLOG_LEVEL_WARN 			__XLOG_LEVEL_WARN
#define XLOG_LEVEL_INFO 			__XLOG_LEVEL_INFO
#define XLOG_LEVEL_DEBUG 			__XLOG_LEVEL_DEBUG
#define XLOG_LEVEL_VERBOSE 			__XLOG_LEVEL_VERBOSE

/** xlog level control options */
#define XLOG_LEVEL_ORECURSIVE 		BIT_MASK(0)
#define XLOG_LEVEL_OFORCE			BIT_MASK(1)

/** xlog context options */
#define XLOG_CONTEXT_OALIVE			BIT_MASK(0)
#define XLOG_CONTEXT_OCOLOR			BIT_MASK(1)
#define XLOG_CONTEXT_OCOLOR_CLASS	BIT_MASK(2)
#define XLOG_CONTEXT_OCOLOR_BODY	BIT_MASK(3)
#define XLOG_CONTEXT_OAUTO_DUMP		BIT_MASK(4)

/** xlog context open/close control options */
#define XLOG_OPEN_LOAD				BIT_MASK(0)
#define XLOG_CLOSE_CLEAR			BIT_MASK(0)

/** xlog printer types */
#define XLOG_PRINTER_STDOUT			XLOG_PRINTER_TYPE_OPT(0)
#define XLOG_PRINTER_STDERR			XLOG_PRINTER_TYPE_OPT(1)
#define XLOG_PRINTER_FILES_BASIC	XLOG_PRINTER_TYPE_OPT(3)
#define XLOG_PRINTER_FILES_ROTATING	XLOG_PRINTER_TYPE_OPT(4)
#define XLOG_PRINTER_FILES_DAILY	XLOG_PRINTER_TYPE_OPT(5)
#define XLOG_PRINTER_RINGBUF		XLOG_PRINTER_TYPE_OPT(6)

#define XLOG_PRINTER_BUFF_NONE		XLOG_PRINTER_BUFF_OPT(0)
#define XLOG_PRINTER_BUFF_RINGBUF	XLOG_PRINTER_BUFF_OPT(1)

/** xlog format control options */
#define XLOG_FORMAT_OTIME			BIT_MASK(0) /**< time */
#define XLOG_FORMAT_OTASK			BIT_MASK(1) /**< task */
#define XLOG_FORMAT_OLEVEL			BIT_MASK(2) /**< level */
#define XLOG_FORMAT_OMODULE			BIT_MASK(3) /**< module */
#define XLOG_FORMAT_OFILE			BIT_MASK(4) /**< file */
#define XLOG_FORMAT_OFUNC			BIT_MASK(5) /**< function */
#define XLOG_FORMAT_OLINE			BIT_MASK(6) /**< line number */
#define XLOG_FORMAT_OLOCATION		BITS_MASK(4, 7)
#define XLOG_FORMAT_OALL	 		BITS_MASK(0, 7)


/** xlog shell list control options */
#define XLOG_LIST_OWITH_TAG			BIT_MASK(0)
#define XLOG_LIST_OONLY_DROP		BIT_MASK(1)
#define XLOG_LIST_OALL				BIT_MASK(2)

/**
 * @brief  open module under parent
 *
 * @param  name, module name('/' to create recursively)
 *         level, default logging level
 *         parent, direct master of module to create
 * @return pointer to `xlog_module_t`, NULL if failed to create module.
 *
 */
XLOG_PUBLIC( xlog_module_t * ) xlog_module_open( const char *name, int level, xlog_module_t *parent );

/**
 * @brief  close module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return error code.
 * @node   set `module` to NULL to avoid wild pointer
 *
 */
XLOG_PUBLIC( int ) xlog_module_close( xlog_module_t *module );

/**
 * @brief  lookup module by name
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return pointer to `xlog_module_t`, NULL if failed.
 *
 */
XLOG_PUBLIC( xlog_module_t * ) xlog_module_lookup( const xlog_module_t *root, const char *name );

/**
 * @brief  get context of xlog module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return xlog context. NULL if failed to get xlog context.
 *
 */
XLOG_PUBLIC( xlog_t * ) xlog_module_context( const xlog_module_t *module );

/**
 * @brief  get output level
 *
 * @param  module, pointer to `xlog_module_t`
 * @return output level.
 *
 */
XLOG_PUBLIC( int ) xlog_module_level_limit( const xlog_module_t *module );

/**
 * @brief  get name of module
 *
 * @param  buffer/length, buffer to save module name
 *         module, pointer to `xlog_module_t`
 * @return module name. full name if buffer given.
 *
 */
XLOG_PUBLIC( const char * ) xlog_module_name( char *buffer, int length, const xlog_module_t *module );

/**
 * @brief  list sub-modules
 *
 * @param  module, pointer to `xlog_module_t`
 *         options, print options
 *
 */
XLOG_PUBLIC( void ) xlog_module_list_submodules( const xlog_module_t *module, int options );

/**
 * @brief  change level of module
 *         child(ren)'s or parent's level may be changed too.
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_set_level( xlog_module_t *module, int level, int flags );

/**
 * @brief  dump module's config to filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         savepath, path to save config; NULL to dump to default path.
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_dump_to( const xlog_module_t *module, const char *savepath );

/**
 * @brief  load module's config from filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         loadpath, file path to load config.
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_load_from( xlog_module_t *module, const char *loadpath );



/**
 * @brief  create xlog context
 *
 * @param  savepath, path to save configurations
 *         option, open option
 * @return pointer to `xlog_t`, NULL if failed to create context.
 *
 */
XLOG_PUBLIC( xlog_t * ) xlog_open( const char *savepath, int option );

/**
 * @brief  destory xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         option, close option
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_close( xlog_t *context, int option );

/**
 * @brief  get xlog version
 *
 * @param  buffer/size, buffer to save version description
 * @return version code[MSB...8 major, 7...4 minor, 3...0 revision]
 *
 */
XLOG_PUBLIC( int ) xlog_version( char *buffer, int size );

/**
 * @brief  list all modules under xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         options, print options
 *
 */
XLOG_PUBLIC( void ) xlog_list_modules( const xlog_t *context, int options );



/**
 * @brief  get default printer
 *
 * @return pointer to printer
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_default( void );

/**
 * @brief  set default printer
 *
 * @param  printer, printer you'd like be the default
 *
 * @return pointer to default printer.
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_set_default( xlog_printer_t *printer );

/**
 * @brief  create dynamic printer
 *
 * @param  options, options to create printer
 * @return pointer to printer
 *
 */
XLOG_PUBLIC( xlog_printer_t * ) xlog_printer_create( int options, ... );

/**
 * @brief  destory created printer
 *
 * @param  printer, pointer to created printer
 * @return error code
 *
 * @note   you MUST call to destory printer dynamically created.
 *
 */
XLOG_PUBLIC( int ) xlog_printer_destory( xlog_printer_t *printer );



/**
 * @brief  output raw log
 *
 * @param  printer, printer to output log
 *         prefix/suffix, prefix and suffix add to the raw log if NOT NULL
 *         context, xlog context
 * @return length of logging.
 *
 */
XLOG_PUBLIC( int ) xlog_output_rawlog(
	xlog_printer_t *printer, xlog_t *context, const char *prefix, const char *suffix,
	const char *format, ...
);

/**
 * @brief  output formated log
 *
 * @param  printer, printer to output log
 *         module, logging module
 *         level, logging level
 *         file/func/line, source location
 * @return length of logging.
 *
 */
XLOG_PUBLIC( int ) xlog_output_fmtlog(
	xlog_printer_t *printer,
	xlog_module_t *module, int level,
	const char *file, const char *func, long int line,
	const char *format, ...
);



/*
 * Usage: debug [OPTIONS] MODULE[/SUB-MODULE/...]
 *
 * Mandatory arguments to long options are mandatory for short options too.
 *   -f, --force        Update module's paramters forcibly,
 *                      minimal changes will applied to it's parent.
 *   -F                 Make no changes on it's parent, contrary to --force option.
 *   -r, --recursive    Update sub-modules too.
 *   -l, --level=LEVEL  Specify the logging level. XLOG_LEVEL_DEBUG if not specified.
 *                      s[ilent]/f[atal]/e[rror]/w[arn]/i[nfo]/d[ebug]/v[erbose](case insensitive, or -1 ~ 5).
 *   -l, --list         List modules in your application.
 *   -a, --all          Show all modules, include the hidden.
 *       --only         Only enable output of specified modules(disabling will be applied to other modules).
 *   -v, --version      Show version of logger.
 *   -h, --help         Display this help and exit.
 */
XLOG_PUBLIC( int ) xlog_shell_main( xlog_t *context, int argc, char **argv );



/**
 * NOTE:
 *   per-defined XLOG_CONTEXT will suppress auto creation of default context
 *   even XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT was defined
 */
#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT) && !(defined XLOG_CONTEXT)
#define XLOG_CONTEXT	NULL
#endif

#if !(defined XLOG_PRINTER)
#define XLOG_PRINTER 	NULL
#endif

#define log_r(...)	xlog_output_rawlog( XLOG_PRINTER, NULL, NULL, NULL, __VA_ARGS__ )

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_FATAL
#define log_f(...)  xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_FATAL, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_f(...)
#endif

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_ERROR
#define log_e(...)	xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_e(...)
#endif

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_WARN
#define log_w(...)	xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_WARN, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_w(...)
#endif

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_INFO
#define log_i(...)	xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_i(...)
#endif

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_DEBUG
#define log_d(...)	xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_d(...)
#endif

#if XLOG_LIMIT_LEVEL_FACTORY >= XLOG_LEVEL_VERBOSE
#define log_v(...)	xlog_output_fmtlog( XLOG_PRINTER, XLOG_MODULE, XLOG_LEVEL_VERBOSE, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#else
#define log_v(...)
#endif

#ifdef __cplusplus
}
#endif

#endif
