## xlog
[![license](https://img.shields.io/badge/license-MIT-brightgreen.svg?style=flat)](https://github.com/vxfury/xlog/blob/master/LICENSE)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/vxfury/xlog?color=red&label=release)
[![Build Status](https://travis-ci.org/vxfury/xlog.svg?branch=master)](https://travis-ci.org/vxfury/xlog)
[![codecov](https://codecov.io/gh/vxfury/xlog/branch/master/graph/badge.svg)](https://codecov.io/gh/vxfury/xlog)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/vxfury/xlog/pulls)

High performance Extensible logging library designed for log formating and dynamic management via CLI(Command Line Interface) or API(Application Programming Interface).

![image](https://github.com/vxfury/logger/raw/master/doc/example.jpg)

## Command Line Interface

``` C
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
XLOG_PUBLIC(int) xlog_shell_main( xlog_t *context, int argc, char **argv );
```

## Application Programming Interface
``` C
/**
 * @brief  open module under parent
 *
 * @param  name, module name('/' to create recursively)
 *         level, default logging level
 *         parent, direct master of module to create
 * @return pointer to `xlog_module_t`, NULL if failed to create module.
 *
 */
XLOG_PUBLIC(xlog_module_t *) xlog_module_open( const char *name, int level, xlog_module_t *parent );

/**
 * @brief  close module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return error code.
 * @node   set `module` to NULL to avoid wild pointer
 *
 */
XLOG_PUBLIC(int) xlog_module_close( xlog_module_t *module );

/**
 * @brief  lookup module by name
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return pointer to `xlog_module_t`, NULL if failed.
 *
 */
XLOG_PUBLIC(xlog_module_t *) xlog_module_lookup( const xlog_module_t *root, const char *name );

/**
 * @brief  get output level
 *
 * @param  module, pointer to `xlog_module_t`
 * @return output level.
 *
 */
XLOG_PUBLIC(int) xlog_module_level_limit( const xlog_module_t *module );

/**
 * @brief  get name of module
 *
 * @param  buffer/length, buffer to save module name
 *         module, pointer to `xlog_module_t`
 * @return module name. full name if buffer given.
 *
 */
XLOG_PUBLIC(const char *) xlog_module_name( char *buffer, int length, const xlog_module_t *module );

/**
 * @brief  get node dir of module
 *
 * @param  buffer/length, buffer to save node dir
 *         module, pointer to `xlog_module_t`
 * @return node dir.
 *
 */
XLOG_PUBLIC(const char *) xlog_module_node_dir( char *buffer, int length, const xlog_module_t *module );

/**
 * @brief  list sub-modules
 *
 * @param  module, pointer to `xlog_module_t`
 *         list, print options
 *
 */
XLOG_PUBLIC(void) xlog_module_list_submodules( const xlog_module_t *module, int mask );

/**
 * @brief  change level of module
 *         child(ren)'s or parent's level may be changed too.
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return error code.
 *
 */
XLOG_PUBLIC(int) xlog_module_set_level( xlog_module_t *module, int level, int flags );

/**
 * @brief  get context of xlog module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return xlog context. NULL if failed to get xlog context.
 *
 */
XLOG_PUBLIC(xlog_t *) xlog_module_context( const xlog_module_t *module );

/**
 * @brief  dump module's config to filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         savepath, path to save config; NULL to dump to default path.
 * @return error code.
 *
 */
XLOG_PUBLIC(int) xlog_module_dump_to( const xlog_module_t *module, const char *savepath );

/**
 * @brief  load module's config from filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         loadpath, file path to load config.
 * @return error code.
 *
 */
XLOG_PUBLIC(int) xlog_module_load_from( xlog_module_t *module, const char *loadpath );


/**
 * @brief  create xlog context
 *
 * @param  savepath, path to save configurations
 *         option, open option
 * @return pointer to `xlog_t`, NULL if failed to create context.
 *
 */
XLOG_PUBLIC(xlog_t *) xlog_open( const char *savepath, int option );

/**
 * @brief  destory xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         option, close option
 * @return error code.
 *
 */
XLOG_PUBLIC(int) xlog_close( xlog_t *context, int option );

/**
 * @brief  list all modules under xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         mask, print options
 *
 */
XLOG_PUBLIC(void) xlog_list_modules( const xlog_t *context, int mask );

/**
 * @brief  get xlog version
 *
 * @param  buffer/size, buffer to save version description
 * @return version code[MSB...8 major, 7...4 minor, 3...0 revision]
 *
 */
XLOG_PUBLIC(int) xlog_version( char *buffer, int size );

/**
 * @brief  get printer by id
 *
 * @param  id, printer id
 * @return pointer to printer
 *
 */
XLOG_PUBLIC(xlog_printer_t *) xlog_printer_get( int id );

/**
 * @brief  output raw log
 *
 * @param  printer, printer to output log
 *         context, xlog context
 * @return length of logging.
 *
 */
XLOG_PUBLIC(int) xlog_output_rawlog(
    xlog_printer_t *printer, xlog_t *context,
    const char *format, ...
);

/**
 * @brief  output formated log
 *
 * @param  printer, printer to output log
 *         context, xlog context
 *         module, logging module
 *         level, logging level
 *         file/func/line, source location
 * @return length of logging.
 *
 */
XLOG_PUBLIC(int) xlog_output_fmtlog(
	xlog_printer_t *printer,
    xlog_t *context, xlog_module_t *module, int level,
    const char *file, const char *func, long int line,
    const char *format, ...
);
```
