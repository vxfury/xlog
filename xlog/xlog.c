#include <xlog/xlog.h>
#include <xlog/xlog_helper.h>

#include "internal.h"
#include "plugins/family_tree.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wembedded-directive"
#endif

#define XLOG_NODE_NAME				".node"
#define XLOG_PERM_MODULE_DIR		0740
#define XLOG_PERM_MODULE_NODE		0640

#define XLOG_MODULE_FROM_NODE(node)	((xlog_module_t *)FAMILY_TREE_DATA( node ))
#define XLOG_MODULE_TO_NODE(module) ((family_tree_t *)FAMILY_TREE_NODE( module ))

#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
static xlog_t *__default_context = NULL;
#endif

#define XLOG_LEVEL_ATTR_DEFAULT(LEVEL) \
	[XLOG_LEVEL_##LEVEL] = { \
		.format = XLOG_DEFAULT_FORMAT_##LEVEL, \
		.time_prefix = XLOG_TAG_PREFIX_LOG_TIME,\
		.time_suffix = XLOG_TAG_SUFFIX_LOG_TIME,\
		.class_prefix = XLOG_TAG_PREFIX_LOG_CLASS( LEVEL ), \
		.class_suffix = XLOG_TAG_SUFFIX_LOG_CLASS( LEVEL ), \
		.body_prefix  = XLOG_TAG_PREFIX_LOG_BODY( LEVEL ), \
		.body_suffix  = XLOG_TAG_PREFIX_LOG_BODY( LEVEL ), \
	}
static const xlog_level_attr_t _level_attributes_default[] = {
	XLOG_LEVEL_ATTR_DEFAULT( FATAL ),
	XLOG_LEVEL_ATTR_DEFAULT( ERROR ),
	XLOG_LEVEL_ATTR_DEFAULT( WARN ),
	XLOG_LEVEL_ATTR_DEFAULT( INFO ),
	XLOG_LEVEL_ATTR_DEFAULT( DEBUG ),
	XLOG_LEVEL_ATTR_DEFAULT( VERBOSE ),
};

#define XLOG_LEVEL_ATTR_NONE(LEVEL) \
	[XLOG_LEVEL_##LEVEL] = { \
		.format = XLOG_DEFAULT_FORMAT_##LEVEL, \
		.time_prefix = XLOG_TAG_PREFIX_LOG_TIME_NONE,\
		.time_suffix = XLOG_TAG_SUFFIX_LOG_TIME_NONE,\
		.class_prefix = XLOG_TAG_PREFIX_LOG_CLASS_NONE( WARN ), \
		.class_suffix = XLOG_TAG_SUFFIX_LOG_CLASS_NONE( LEVEL ), \
		.body_prefix  = XLOG_TAG_PREFIX_LOG_BODY_NONE( LEVEL ), \
		.body_suffix  = XLOG_TAG_PREFIX_LOG_BODY_NONE( LEVEL ), \
	}
static const xlog_level_attr_t _level_attributes_none[] = {
	XLOG_LEVEL_ATTR_NONE( FATAL ),
	XLOG_LEVEL_ATTR_NONE( ERROR ),
	XLOG_LEVEL_ATTR_NONE( WARN ),
	XLOG_LEVEL_ATTR_NONE( INFO ),
	XLOG_LEVEL_ATTR_NONE( DEBUG ),
	XLOG_LEVEL_ATTR_NONE( VERBOSE ),
};

/** reverse version of strncpy */
static char *__xlog_strnrcpy( char *dst, size_t size, const char *src )
{
	XLOG_ASSERT( dst );
	XLOG_ASSERT( src );
	
	const char *cur = src + strlen( src ) - 1;
	while( size-- && cur >= src ) {
		*dst-- = *cur--;
	}
	
	return dst;
}

/** copy a C-string into a sized buffer */
static size_t __xlog_strlcpy( char *dest, const char *src, size_t size )
{
	size_t slen = strlen( src );
	
	if( size ) {
		size_t clen = ( slen >= size ) ? size - 1 : slen;
		memcpy( dest, src, clen );
		dest[clen] = '\0';
	}
	
	return slen;
}

/** make directory recursively */
static int __xlog_mkdir_p( const char *dir, mode_t mode )
{
	XLOG_ASSERT( dir );
	int status = 0;
	char pathparent[1024], *pathcursor = pathparent;
	char *__dir = strdup( dir ), *__ptr, *__saveptr;
	const char *__delim = "/\\";
	
	__ptr = strtok_r( __dir, __delim, &__saveptr );
	while( __ptr != NULL ) {
		pathcursor += snprintf(
	        pathcursor, pathparent + sizeof( pathparent ) - pathcursor,
	        "%s/", __ptr
	    );
		#define __X( path ) ( 0 == strcmp( __ptr, path ) )
		if( !__X( "." ) && !__X( ".." ) ) { // for ".." or ".", just skip
			if( access( pathparent, F_OK ) != 0 ) {
				if( ( status = mkdir( pathparent, mode ) ) != 0 ) {
					log_r( "failed to create directory(%s), 'cause %s\n", pathparent, strerror( errno ) );
					break;
				}
			}
		}
		
		__ptr = strtok_r( NULL, __delim, &__saveptr );
	}
	free( __dir );
	
	return status;
}

/** remove directory recursively */
static int __xlog_rmdir_r( const char *dir )
{
	XLOG_ASSERT( dir );
	int status = 0;
	char dirtmp[1024];
	struct dirent *dp;
	struct stat dirstat;
	
	if( access( dir, F_OK ) != 0 ) {
		return 0;
	}
	
	if( stat( dir, &dirstat ) < 0 ) {
		return -1;
	}
	
	if( S_ISREG( dirstat.st_mode ) ) {
		status = remove( dir );
	} else if( S_ISDIR( dirstat.st_mode ) ) {
		DIR *dirp = opendir( dir );
		
		while( ( dp = readdir( dirp ) ) != NULL ) {
			if( ( 0 == strcmp( ".", dp->d_name ) ) || ( 0 == strcmp( "..", dp->d_name ) ) ) {
				continue;
			}
			
			snprintf( dirtmp, sizeof( dirtmp ), "%s/%s", dir, dp->d_name );
			status = __xlog_rmdir_r( dirtmp );
			if( status != 0 ) {
				break;
			}
		}
		closedir( dirp );
		
		if( 0 == status ) {
			status = rmdir( dir );
		}
	} else {
		return -2;
	}
	
	return status;
}

/** short name of level, return "#" if not legal level */
static const char *__xlog_level_short_name( int level )
{
	static const char *lvl_tags[] = {
		[XLOG_LEVEL_FATAL]   = XLOG_TAG_LEVEL_FATAL,
		[XLOG_LEVEL_ERROR]   = XLOG_TAG_LEVEL_ERROR,
		[XLOG_LEVEL_WARN]    = XLOG_TAG_LEVEL_WARN,
		[XLOG_LEVEL_INFO]    = XLOG_TAG_LEVEL_INFO,
		[XLOG_LEVEL_DEBUG]   = XLOG_TAG_LEVEL_DEBUG,
		[XLOG_LEVEL_VERBOSE] = XLOG_TAG_LEVEL_VERBOSE,
	};
	
	if( XLOG_IF_LEGAL_LEVEL( level ) ) {
		return lvl_tags[level];
	} else {
		XLOG_TRACE( "Illegal level, return default short name." );
		return "#";
	}
}

/** check if format is enabled for specified level */
static bool __xlog_format_been_enabled( const xlog_module_t *module, int level, int format )
{
	XLOG_ASSERT( XLOG_IF_LEGAL_LEVEL( level ) );
	
	if( module ) {
		xlog_t *context = xlog_module_context( module );
		
		if(
		    context
		    && ( context->options & XLOG_CONTEXT_OALIVE )
		    && ( context->attributes[level].format & format )
		) {
			return true;
		}
		
		return false;
	} else {
		return true;
	}
}

/** lookup module in root tree, for checking if module existed */
static inline xlog_module_t *__xlog_module_lookup( const char *name, const family_tree_t *root_tree )
{
	XLOG_ASSERT( name );
	XLOG_ASSERT( root_tree );
	const family_tree_t *te_node = root_tree->child;
	while( te_node ) {
		xlog_module_t *module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( te_node );
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		if( module->magic != XLOG_MAGIC_MODULE ) {
			XLOG_TRACE( "Runtime error: may be module has been closed." );
			return NULL;
		}
		#endif
		if( strcmp( module->name, name ) == 0 ) {
			return module;
		}
		te_node = te_node->next;
	}
	XLOG_TRACE( "Module named %s not exists.", name );
	return NULL;
}

/** create module under parent, NO '/' in name */
static inline xlog_module_t *__xlog_module_open( const char *name, int level, xlog_module_t *parent )
{
	family_tree_t *te_parent = parent ? ( family_tree_t * )XLOG_MODULE_TO_NODE( parent ) : NULL;
	xlog_module_t *module = parent ? __xlog_module_lookup( name, te_parent ) : NULL;
	if( module ) {
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		if( module->magic != XLOG_MAGIC_MODULE ) {
			XLOG_TRACE( "Runtime error: may be module has been closed." );
			return NULL;
		}
		#endif
		XLOG_TRACE( "Module already exists, no changes to it." );
		return module;
	}
	
	family_tree_t *te_module = family_tree_create( sizeof( xlog_module_t ) );
	if( te_module ) {
		module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( te_module );
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		module->magic = XLOG_MAGIC_MODULE;
		#endif
		pthread_mutex_init( &module->lock, NULL );
		module->level = level;
		module->name = name ? XLOG_STRDUP( name ) : NULL;
		module->context = parent ? parent->context : NULL;
		XLOG_STATS_INIT( &module->stats, XLOG_STATS_MODULE_OPTION );
		if( te_parent ) {
			family_tree_set_parent( te_module, te_parent );
		}
	} else {
		XLOG_TRACE( "Failed to create tree node: %s", strerror( errno ) );
	}
	
	return module;
}

/** hook to destory lock before close module */
static inline void __xlog_module_destory_hook( family_tree_t *node )
{
	if( node ) {
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		if( XLOG_MODULE_FROM_NODE( node )->magic != XLOG_MAGIC_MODULE ) {
			XLOG_TRACE( "Runtime error: may be module has been closed." );
			return;
		}
		XLOG_MODULE_FROM_NODE( node )->magic = 0;
		#endif
		xlog_module_t *module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( node );
		XLOG_FREE( module->name );
		XLOG_STATS_FINI( &module->stats );
		pthread_mutex_destroy( &module->lock );
	} else {
		XLOG_TRACE( "Invalid tree node." );
	}
}

/**
 * @brief  open module under parent
 *
 * @param  name, module name('/' to create recursively)
 *         level, default logging level
 *         parent, direct master of module to create
 * @return pointer to `xlog_module_t`, NULL if failed to create module.
 *
 */
XLOG_PUBLIC( xlog_module_t * ) xlog_module_open( const char *name, int level, xlog_module_t *parent )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( parent && parent->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return NULL;
	}
	#endif
	#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	if( parent == NULL ) {
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
				return 0;
			}
		}
		XLOG_TRACE( "Redirected to default context." );
		parent = __default_context->module;
	}
	#endif
	xlog_module_t *module = NULL, *__parent = parent;
	char *__name = name ? XLOG_STRDUP( name ) : NULL, *__ptr, *__saveptr;
	const char *__delim = "/";
	
	if( __name ) {
		__ptr = strtok_r( __name, __delim, &__saveptr );
		while( __ptr != NULL ) {
			XLOG_TRACE( "Create new module named \"%s\" under it's parent.", __ptr );
			module = __xlog_module_open( ( const char * )__ptr, level, __parent );
			__parent = module;
			__ptr = strtok_r( NULL, __delim, &__saveptr );
		}
		XLOG_FREE( __name );
	} else {
		XLOG_TRACE( "Failed to duplicate string or name given is NULL." );
		module = __xlog_module_open( __name, level, __parent );
	}
	
	return module;
}

/**
 * @brief  close module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return error code.
 * @node   set `module` to NULL to avoid wild pointer
 *
 */
int xlog_module_close( xlog_module_t *module )
{
	if( module ) {
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		if( module->magic != XLOG_MAGIC_MODULE ) {
			XLOG_TRACE( "Runtime error: may be module has been closed." );
			return EINVAL;
		}
		#endif
		family_tree_t *te_module = ( family_tree_t * )XLOG_MODULE_TO_NODE( module );
		family_tree_destory_with_hook( te_module, __xlog_module_destory_hook );
	} else {
		XLOG_TRACE( "Module given is NULL." );
		return EINVAL;
	}
	
	return 0;
}

/**
 * @brief  lookup module by name
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return pointer to `xlog_module_t`, NULL if failed.
 *
 */
XLOG_PUBLIC( xlog_module_t * ) xlog_module_lookup( const xlog_module_t *root, const char *name )
{
	XLOG_ASSERT( name );
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( root && root->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return NULL;
	}
	#endif
	#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	if( root == NULL ) {
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
				return 0;
			}
		}
		XLOG_TRACE( "Redirected to default context." );
		root = __default_context->module;
	}
	#endif
	const xlog_module_t *module = NULL, *__root = root;
	
	const char *delim = "/";
	char *__name = XLOG_STRDUP( name ), *ptr = NULL, *saveptr = NULL;
	
	ptr = strtok_r( __name, delim, &saveptr );
	while( ptr != NULL && __root != NULL ) {
		xlog_module_t *__module = __xlog_module_lookup( ptr, ( const family_tree_t * )XLOG_MODULE_TO_NODE( __root ) );
		if( __module ) {
			XLOG_TRACE( "Parent Module found, continue to search deeper." );
			__root = __module;
			module = __root;
		} else {
			XLOG_TRACE( "Parent Module mismatch, continue to search next." );
			module = NULL;
			family_tree_t *__node = ( ( family_tree_t * )XLOG_MODULE_TO_NODE( __root ) )->next;
			if( __node == NULL ) {
				break;
			}
			__root = ( const xlog_module_t * )XLOG_MODULE_FROM_NODE( __node );
		}
		ptr = strtok_r( NULL, delim, &saveptr );
	}
	
	XLOG_FREE( __name );
	
	return ( xlog_module_t * )module;
}

/**
 * @brief  get context of xlog module
 *
 * @param  module, pointer to `xlog_module_t`
 * @return xlog context. NULL if failed to get xlog context.
 *
 */
XLOG_PUBLIC( xlog_t * ) xlog_module_context( const xlog_module_t *module )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module && module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return NULL;
	}
	#endif
	if( module == NULL ) {
		#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
			}
		}
		return __default_context;
		#else
		return NULL;
		#endif
	} else {
		return ( xlog_t * )module->context;
	}
}

/**
 * @brief  get output level
 *
 * @param  module, pointer to `xlog_module_t`
 * @return output level.
 *
 */
XLOG_PUBLIC( int ) xlog_module_level_limit( const xlog_module_t *module )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module && module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return XLOG_LEVEL_SILENT;
	}
	#endif
	int level = XLOG_LEVEL_VERBOSE;
	const xlog_module_t *cur = module;
	while( cur ) {
		if( XLOG_IF_DROP_LEVEL( level, cur->level ) ) {
			XLOG_TRACE( "Log in this level will be dropped." );
			level = cur->level;
		}
		family_tree_t *__node = family_tree_parent( ( const family_tree_t * )XLOG_MODULE_TO_NODE( cur ) );
		cur = __node ? ( const xlog_module_t * )XLOG_MODULE_FROM_NODE( __node ) : NULL;
	}
	
	return level;
}

/**
 * @brief  get name of module
 *
 * @param  buffer/length, buffer to save module name
 *         module, pointer to `xlog_module_t`
 * @return module name. full name if buffer given.
 *
 */
XLOG_PUBLIC( const char * ) xlog_module_name( char *buffer, int length, const xlog_module_t *module )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module && module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return NULL;
	}
	#endif
	if( NULL == buffer || 0 == length ) {
		XLOG_TRACE( "invalid parameters." );
		return module ? module->name : NULL;
	} else {
		char *ptr = buffer + length - 1;
		*ptr -- = '\0';
		
		const xlog_module_t *cur = module;
		while( cur ) {
			if( cur->name ) {
				ptr = __xlog_strnrcpy( ptr, ptr - buffer, cur->name );
				ptr = __xlog_strnrcpy( ptr, ptr - buffer, "/" );
			} else {
				XLOG_TRACE( "Name of current module is NULL, pay attention to it." );
			}
			family_tree_t *__node = family_tree_parent( ( const family_tree_t * )XLOG_MODULE_TO_NODE( cur ) );
			cur = __node ? ( const xlog_module_t * )XLOG_MODULE_FROM_NODE( __node ) : NULL;
		}
		memcpy( buffer, ptr + 1, ptr - buffer );
		
		return buffer;
	}
}

/** get node dir of module */
static const char *__xlog_module_node_dir( char *buffer, int length, const xlog_module_t *module )
{
	xlog_t *context = xlog_module_context( module );
	if( context == NULL || context->savepath == NULL ) {
		XLOG_TRACE( "context is NULL, or context->savepath is NULL." );
		return NULL;
	}
	
	size_t len = strlen( context->savepath );
	memcpy( buffer, context->savepath, len + 1 );
	xlog_module_name( buffer + len, length - len, module );
	
	return buffer;
}

/**
 * @brief  list sub-modules
 *
 * @param  module, pointer to `xlog_module_t`
 *         options, print options
 *
 */
XLOG_PUBLIC( void ) xlog_module_list_submodules( const xlog_module_t *module, int options )
{
	if( module == NULL ) {
		XLOG_TRACE( "Invalid module" );
		return;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return;
	}
	#endif
	family_tree_t *child = ( ( family_tree_t * )XLOG_MODULE_TO_NODE( module ) )->child;
	if( child == NULL ) {
		return;
	}
	int __limit;
	xlog_module_t *__module = NULL;
	char modulepath[XLOG_LIMIT_MODULE_PATH];
	
	while( child ) {
		__module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( child );
		
		// Feature(Hidden modules): don't show module whose name start with dot[.]
		if(
		    __module->name[0] == '.' && !( options & XLOG_LIST_OALL )
		) {
			XLOG_TRACE( "Hiddent module[%s] won't be printed to session.", __module->name );
			continue;
		}
		
		__limit = xlog_module_level_limit( __module );
		const char *name = xlog_module_name( modulepath, XLOG_LIMIT_MODULE_PATH, __module );
		
		bool show = true;
		if( ( options & XLOG_LIST_OONLY_DROP ) ) {
			if( !XLOG_IF_DROP_LEVEL( __module->level, __limit ) ) {
				show = false;
			}
		}
		
		if( show ) {
			if( options & XLOG_LIST_OWITH_TAG ) {
				log_r(
				    "[%s]%c %s\n"
				    , __xlog_level_short_name( __module->level )
				    , XLOG_IF_DROP_LEVEL( __module->level, __limit ) ? '*' : ' '
				    , name
				);
			} else {
				log_r( "%s\n", name );
			}
		}
		
		xlog_module_list_submodules( __module, options );
		
		child = child->next;
	}
}

/**
 * @brief  change level of module
 *         child(ren)'s or parent's level may be changed too.
 *
 * @param  root, lookup under this module
 *         name, module name
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_set_level( xlog_module_t *module, int level, int flags )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module && module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return EINVAL;
	}
	#endif
	#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	if( module == NULL ) {
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
				return ENOMEM;
			}
		}
		XLOG_TRACE( "Redirected to default context." );
		module = __default_context->module;
	}
	#endif
	xlog_t *context = xlog_module_context( module );
	if( module && XLOG_IF_LEGAL_LEVEL( level ) ) {
		module->level = level;
		if( context && ( context->options & XLOG_CONTEXT_OAUTO_DUMP ) ) {
			XLOG_TRACE( "Auto dump enabled, dump to file now." );
			xlog_module_dump_to( module, NULL );
		}
	} else {
		return -1;
	}
	family_tree_t *__node = NULL;
	xlog_module_t *__module = NULL;
	
	if( flags & XLOG_LEVEL_ORECURSIVE ) {
		__node = ( ( family_tree_t * )XLOG_MODULE_TO_NODE( module ) )->child;
		while( __node ) {
			__module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( __node );
			__module->level = level;
			if( context && ( context->options & XLOG_CONTEXT_OAUTO_DUMP ) ) {
				xlog_module_dump_to( __module, NULL );
			}
			
			if( __node->child ) {
				xlog_module_set_level( ( xlog_module_t * )XLOG_MODULE_FROM_NODE( __node->child ), level, flags & ( ~ XLOG_LEVEL_OFORCE ) );
			}
			
			__node = __node->next;
		}
		
		__node = NULL;
		__module = NULL;
	}
	
	if( flags & XLOG_LEVEL_OFORCE ) {
		__node = family_tree_parent( ( const family_tree_t * )XLOG_MODULE_TO_NODE( module ) );
		while( __node ) {
			__module = ( xlog_module_t * )XLOG_MODULE_FROM_NODE( __node );
			if( XLOG_IF_LOWER_LEVEL( __module->level, level ) ) {
				__module->level = level;
				if( context && ( context->options & XLOG_CONTEXT_OAUTO_DUMP ) ) {
					xlog_module_dump_to( __module, NULL );
				}
			}
			__node = family_tree_parent( __node );
		}
	}
	
	return 0;
}


typedef struct {
	int level;
	struct {
		int option;
		unsigned int data[XLOG_STATS_LENGTH_MAX];
	} stats;
} xlog_module_node_t;

/**
 * @brief  dump module's config to filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         savepath, path to save config; NULL to dump to default path.
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_dump_to( const xlog_module_t *module, const char *savepath )
{
	if( module == NULL ) {
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return EINVAL;
	}
	#endif
	
	int fd = -1;
	xlog_t *context = xlog_module_context( module );
	if( context && savepath == NULL ) {
		if( context->savepath == NULL ) {
			XLOG_TRACE( "context->savepath is NULL, break now." );
			return EINVAL;
		}
		char path[XLOG_LIMIT_NODE_PATH];
		if( __xlog_module_node_dir( path, sizeof( path ), module ) ) {
			__xlog_mkdir_p( path, XLOG_PERM_MODULE_DIR );
			strcat( path, "/" XLOG_NODE_NAME );
			
			fd = open( path, O_WRONLY | O_CREAT, XLOG_PERM_MODULE_NODE );
		} else {
			XLOG_TRACE( "failed to get node path: buffer is too small." );
			return EINVAL;
		}
	} else {
		XLOG_TRACE( "open file: %s.", savepath );
		fd = open( savepath, O_WRONLY | O_CREAT, XLOG_PERM_MODULE_NODE );
	}
	
	if( fd >= 0 ) {
		xlog_module_node_t config;
		config.level = module->level;
		#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_MODULE)
		config.stats.option = module->stats.option;
		size_t stats_size = sizeof( unsigned int ) * XLOG_STATS_LENGTH( module->stats.option );
		memcpy( &config.stats.data, module->stats.data, stats_size );
		if( write(
			fd, &config,
			offsetof(xlog_module_node_t, stats.data) + stats_size
		) != offsetof(xlog_module_node_t, stats.data) + stats_size ) {
			close( fd );
			return EIO;
		}
		#else
		if( write(
			fd, &config,
			offsetof(xlog_module_node_t, stats )
			) != offsetof(xlog_module_node_t, stats )
		) {
			close( fd );
			return EIO;
		}
		#endif
		
		close( fd );
		
		return 0;
	}
	
	XLOG_TRACE( "failed to open file: %s.", strerror( errno ) );
	return errno;
}

/**
 * @brief  load module's config from filesystem
 *
 * @param  module, pointer to `xlog_module_t`
 *         loadpath, file path to load config.
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_module_load_from( xlog_module_t *module, const char *loadpath )
{
	if( module == NULL ) {
		return EINVAL;
	}
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return EINVAL;
	}
	#endif
	
	int fd = -1;
	xlog_t *context = xlog_module_context( module );
	if( context && loadpath == NULL ) {
		char path[XLOG_LIMIT_NODE_PATH];
		if( __xlog_module_node_dir( path, sizeof( path ), module ) ) {
			strcat( path, "/" XLOG_NODE_NAME );
			if( access( path, F_OK ) == 0 ) {
				fd = open( path, O_RDONLY );
			} else {
				XLOG_TRACE( "No such file." );
				return ENFILE;
			}
		} else {
			return EOVERFLOW;
		}
	} else {
		fd = open( loadpath, O_RDONLY );
	}
	
	xlog_module_node_t config;
	if( fd >= 0 ) {
		if( read( fd, &config, sizeof( xlog_module_node_t ) ) > 0 ) {
			close( fd );
			
			XLOG_TRACE( "load config from file ..." );
			pthread_mutex_lock( &module->lock );
			if( XLOG_IF_LEGAL_LEVEL( config.level ) ) {
				XLOG_TRACE( "level = %d, config.level = %d", module->level, config.level );
				module->level = config.level;
			}
			#if (defined XLOG_FEATURE_ENABLE_STATS) && (defined XLOG_FEATURE_ENABLE_STATS_MODULE)
			memcpy( module->stats.data, config.stats.data, sizeof( unsigned int ) * XLOG_STATS_LENGTH( module->stats.option ) );
			#endif
			
			pthread_mutex_unlock( &module->lock );
			
			return 0;
		} else {
			XLOG_TRACE( "read failed: %s.", strerror( errno ) );
			close( fd );
		}
	} else {
		XLOG_TRACE( "failed to open file: %s", strerror( errno ) );
	}
	
	return errno;
}

#include <dirent.h>

static int __traverse_dir_recursive( const char *path, int ( *hook )( const char *, void * ), void *arg )
{
	XLOG_ASSERT( path );
	XLOG_ASSERT( hook );
	DIR *dp = opendir( path );
	if( dp == NULL ) {
		XLOG_TRACE( "opendir failed: %s.", strerror( errno ) );
		return errno;
	}
	struct dirent *entry;
	struct stat statbuf;
	
	char fullpath[256] = { 0 };
	char childpath[XLOG_LIMIT_NODE_PATH] = { 0 };
	while( ( entry = readdir( dp ) ) != NULL ) {
		if(
		    strcmp( entry->d_name, "." ) == 0
		    || strcmp( entry->d_name, ".." ) == 0
		) {
			continue;
		}
		
		if( getcwd( fullpath, sizeof( fullpath ) ) == NULL ) {
			XLOG_TRACE( "path may be truncated, move to next dir." );
			continue;
		}
		strcat( fullpath, "/" );
		strcat( fullpath, path );
		strcat( fullpath, "/" );
		strcat( fullpath, entry->d_name );
		XLOG_TRACE( "fullpath = %s", fullpath );
		if( lstat( fullpath, &statbuf ) != 0 ) {
			{
				char curdir[256];
				if( getcwd( curdir, sizeof( curdir ) ) == NULL ) {
					XLOG_TRACE( "path may be truncated, move to next dir." );
					continue;
				}
				XLOG_TRACE( "curdir is %s", curdir );
			}
			XLOG_TRACE( "stat error: %s, 'cause %s", fullpath, strerror( errno ) );
		}
		if( ( S_IFDIR & statbuf.st_mode ) ) {
			if( snprintf( childpath, sizeof( childpath ), "%s/%s", path, entry->d_name ) > sizeof( childpath ) ) {
				XLOG_TRACE( "path may be truncated, move to next dir." );
				continue;
			}
			hook( childpath, arg );
			__traverse_dir_recursive( childpath, hook, arg );
		} else {
			XLOG_TRACE( "Format: %d.", statbuf.st_mode );
		}
	}
	closedir( dp );
	
	return 0;
}

static int __hook_load_from_file( const char *path, void *arg )
{
	xlog_module_t *root = ( xlog_module_t * )arg;
	xlog_t *context = ( xlog_t * )root->context;
	XLOG_TRACE( "path = %s, dir = %s", path, path + strlen( context->savepath ) );
	xlog_module_t *module = xlog_module_open( path + strlen( context->savepath ), XLOG_LEVEL_INFO, root );
	if( module ) {
		char nodepath[XLOG_LIMIT_NODE_PATH] = { 0 };
		strcpy( nodepath, path );
		strcat( nodepath, "/" XLOG_NODE_NAME );
		XLOG_TRACE( "nodepath = %s", nodepath );
		xlog_module_load_from( module, nodepath );
	}
	
	return 0;
}

/**
 * @brief  create xlog modules from files
 *
 * @param  context, path to save configurations
 *         option, open option
 * @return pointer to `xlog_t`, NULL if failed to create context.
 *
 */
static int __xlog_load_modules( xlog_t *context )
{
	XLOG_ASSERT( context );
	XLOG_ASSERT( context->savepath );
	XLOG_ASSERT( context->module );
	
	xlog_module_load_from( context->module, NULL );
	
	return __traverse_dir_recursive( context->savepath, __hook_load_from_file, context->module );
}

/**
 * @brief  create xlog context
 *
 * @param  savepath, path to save configurations
 *         option, open option
 * @return pointer to `xlog_t`, NULL if failed to create context.
 *
 */
XLOG_PUBLIC( xlog_t * ) xlog_open( const char *savepath, int option )
{
	xlog_t *context = NULL;
	context = ( xlog_t * )XLOG_MALLOC( sizeof( xlog_t ) );
	if( context ) {
		memset( context, 0, sizeof( xlog_t ) );
		
		/** NOTE: initialize lock */
		if( pthread_mutex_init( &context->lock, NULL ) != 0 ) {
			XLOG_FREE( context );
			context = NULL;
		} else {
			context->module = __xlog_module_open( NULL, XLOG_LEVEL_VERBOSE, NULL );
			if( NULL == context->module ) {
				pthread_mutex_destroy( &context->lock );
				XLOG_FREE( context );
				context = NULL;
				
				return context;
			}
			context->module->context = context;
			
			context->options = XLOG_CONTEXT_OALIVE | XLOG_CONTEXT_OCOLOR;
			if( savepath ) {
				XLOG_TRACE( "Auto dump enabled 'cause savepath is NOT NULL." );
				context->options |= XLOG_CONTEXT_OAUTO_DUMP;
			}
			memcpy( context->attributes, &_level_attributes_default, sizeof( context->attributes ) );
			
			/** NOTE: create files to save configurations */
			if( savepath ) {
				char *_dir = XLOG_STRDUP( savepath ), *_ptr = _dir;
				if( _dir ) {
					_ptr = _dir + strlen( _dir ) - 1;
					while( _ptr > _dir && ( '\\' == *_ptr || '/' == *_ptr ) ) {
						XLOG_TRACE( "Strip ending \"/\" and \"\\\"." );
						*_ptr -- = '\0';
					}
					context->savepath = _dir;
					__xlog_mkdir_p( context->savepath, 0777 );
					
					if( option & XLOG_OPEN_LOAD ) {
						XLOG_TRACE( "Loading configuration from files." );
						__xlog_load_modules( context );
					}
				}
			} else {
				XLOG_TRACE( "Set savepath to NULL." );
				context->savepath = NULL;
			}
			
			/** NOTE: initialize stats */
			XLOG_STATS_INIT( &context->stats, XLOG_STATS_CONTEXT_OPTION );
			
			#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
			context->magic = XLOG_MAGIC_CONTEXT;
			#endif
		}
	}
	
	return context;
}

/**
 * @brief  destory xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         option, close option
 * @return error code.
 *
 */
XLOG_PUBLIC( int ) xlog_close( xlog_t *context, int option )
{
	if( context ) {
		#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
		if( context->magic != XLOG_MAGIC_CONTEXT ) {
			XLOG_TRACE( "Runtime error: may be context has been closed." );
			return EINVAL;
		}
		context->magic = XLOG_MAGIC_CONTEXT;
		#endif
		pthread_mutex_lock( &context->lock );
		xlog_module_close( context->module );
		if( context->savepath ) {
			if( option & XLOG_CLOSE_CLEAR ) {
				XLOG_TRACE( "CLEAR ON CLOSE is enabled, and savepath is NOT NULL, remove directories." );
				__xlog_rmdir_r( context->savepath );
			}
			XLOG_FREE( context->savepath );
		}
		XLOG_STATS_FINI( &context->stats );
		pthread_mutex_unlock( &context->lock );
		pthread_mutex_destroy( &context->lock );
		#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
		if( context == __default_context ) {
			XLOG_TRACE( "Destory default context." );
			__default_context = NULL;
		}
		#endif
		XLOG_FREE( context );
	}
	
	return 0;
}

/**
 * @brief  list all modules under xlog context
 *
 * @param  context, pointer to `xlog_t`
 *         options, print options
 *
 */
XLOG_PUBLIC( void ) xlog_list_modules( const xlog_t *context, int options )
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( context && context->magic != XLOG_MAGIC_CONTEXT ) {
		XLOG_TRACE( "Runtime error: may be context has been closed." );
		return;
	}
	#endif
	#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
	if( context == NULL ) {
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
				return;
			}
		}
		XLOG_TRACE( "Redirected to default context." );
		context = __default_context;
	}
	#else
	XLOG_ASSERT( context && context->module );
	#endif
	
	xlog_module_list_submodules( context->module, options );
}

/**
 * @brief  get xlog version
 *
 * @param  buffer/size, buffer to save version description
 * @return version code[MSB...8 major, 7...4 minor, 3...0 revision]
 *
 */
XLOG_PUBLIC( int ) xlog_version( char *buffer, int size )
{
	#if (defined XLOG_VERSION_WITH_BUILDDATE)
	#define DATE_YEAR ( ( ( ( __DATE__[7] - '0' ) * 10 + ( __DATE__[8] - '0' ) ) *10 \
	    + ( __DATE__[9] - '0' ) ) * 10 + ( __DATE__[10] - '0' ) \
	)
	#define DATE_MONTH ( __DATE__[2] == 'n' ? 1 \
	    : __DATE__[2] == 'b' ? 2 \
	    : __DATE__[2] == 'r' ? ( __DATE__[0] == 'M' ? 3 : 4 ) \
	    : __DATE__[2] == 'y' ? 5 \
	    : __DATE__[2] == 'n' ? 6 \
	    : __DATE__[2] == 'l' ? 7 \
	    : __DATE__[2] == 'g' ? 8 \
	    : __DATE__[2] == 'p' ? 9 \
	    : __DATE__[2] == 't' ? 10 \
	    : __DATE__[2] == 'v' ? 11 : 12 \
	)
	#define DATE_DAY ( ( __DATE__[4] == ' ' ? 0 :__DATE__[4] - '0' ) * 10 \
	    + ( __DATE__[5] - '0' ) \
	)
	#endif

	if( buffer != NULL ) {
		snprintf(
		    buffer, size, "V%d.%d.%d"
		    #if (defined XLOG_VERSION_WITH_BUILDDATE)
		    "(build %4d%02d%02d)"
		    #endif
		    , XLOG_VERSION_MAJOR, XLOG_VERSION_MINOR,
		    XLOG_VERSION_PATCH
		    #if (defined XLOG_VERSION_WITH_BUILDDATE)
		    , DATE_YEAR, DATE_MONTH, DATE_DAY
		    #endif
		);
	}
	
	return XLOG_VERSION_MAJOR << 8 | XLOG_VERSION_MINOR << 4 | XLOG_VERSION_PATCH << 0;
}

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
)
{
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( context && context->magic != XLOG_MAGIC_CONTEXT ) {
		__XLOG_TRACE( "Runtime error: may be context has been closed." );
		return 0;
	}
	if( printer && printer->magic != XLOG_MAGIC_PRINTER ) {
		__XLOG_TRACE( "Runtime error: may be printer has been closed." );
		return 0;
	}
	#endif
	if( printer == NULL ) {
		printer = xlog_printer_default();
	}
	
	/** global setting in xlog */
	if( context && !( context->options & XLOG_CONTEXT_OALIVE ) ) {
		__XLOG_TRACE( "Dropped by context." );
		return 0;
	}
	
	int length = 0;
	xlog_payload_t *payload = xlog_payload_create( XLOG_PAYLOAD_ID_AUTO, "Log Text", XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN, 240, 32 );
	if( payload ) {
		if( prefix ) {
			xlog_payload_append_text( &payload, prefix );
		}
		va_list ap;
		va_start( ap, format );
		xlog_payload_append_text_va_list( &payload, format, ap );
		va_end( ap );
		if( suffix ) {
			xlog_payload_append_text( &payload, suffix );
		}
		length = xlog_payload_print_TEXT( payload, printer );
		xlog_payload_destory( &payload );
	} else {
		__XLOG_TRACE( "Failed to create payload." );
	}
	
	return length;
}

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
)
{
	xlog_t *context = xlog_module_context( module );
	if( context == NULL ) {
		#if (defined XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT)
		if( __default_context == NULL ) {
			XLOG_TRACE( "Create default context." );
			__default_context = xlog_open( NULL, 0 );
			if( __default_context == NULL ) {
				XLOG_TRACE( "Failed to create default context." );
				return 0;
			}
		}
		XLOG_TRACE( "Redirected to default context." );
		context = __default_context;
		#else
		return 0;
		#endif
	}
	XLOG_ASSERT( context );
	if( printer == NULL ) {
		XLOG_TRACE( "Output via defualt printer." );
		printer = xlog_printer_default();
	}
	XLOG_ASSERT( printer );
	#if (defined XLOG_POLICY_ENABLE_RUNTIME_SAFE)
	if( context->magic != XLOG_MAGIC_CONTEXT ) {
		XLOG_TRACE( "Runtime error: may be context has been closed." );
		return 0;
	}
	if( printer->magic != XLOG_MAGIC_PRINTER ) {
		XLOG_TRACE( "Runtime error: may be printer has been closed." );
		return 0;
	}
	if( module && module->magic != XLOG_MAGIC_MODULE ) {
		XLOG_TRACE( "Runtime error: may be module has been closed." );
		return 0;
	}
	#endif
	
	XLOG_STATS_UPDATE( &context->stats, REQUEST, INPUT, 1 );
	if( module ) {
		XLOG_STATS_UPDATE( &module->stats, REQUEST, INPUT, 1 );
	}
	
	/** global setting in xlog */
	if( !( context->options & XLOG_CONTEXT_OALIVE ) ) {
		XLOG_STATS_UPDATE( &context->stats, REQUEST, DROPPED, 1 );
		XLOG_TRACE( "Dropped by context." );
		return 0;
	}
	
	/** module based limit */
	if( module && XLOG_IF_DROP_LEVEL( level, xlog_module_level_limit( module ) ) ) {
		XLOG_STATS_UPDATE( &module->stats, REQUEST, DROPPED, 1 );
		XLOG_TRACE( "Dropped by module." );
		return 0;
	}
	
	const xlog_level_attr_t *level_attributes = _level_attributes_none;
	if( printer->optctl ) {
		int optval;
		if(
		   printer->optctl( printer, XLOG_PRINTER_CTRL_GABICLR, &optval, sizeof( int ) ) == 0
		   && optval
		) {
			level_attributes = context->attributes;
		}
	}
	xlog_payload_t *payload = xlog_payload_create(
		XLOG_PAYLOAD_ID_AUTO, "Log",
		XLOG_PAYLOAD_ODYNAMIC | XLOG_PAYLOAD_OALIGN | XLOG_PAYLOAD_OTEXT, XLOG_DEFAULT_PAYLOAD_SIZE, 16
	);
	if( payload == NULL ) {
		XLOG_TRACE( "Failed to create payload." );
		return 0;
	}
	
	/* package time */
	if( __xlog_format_been_enabled( module, level, XLOG_FORMAT_OTIME ) ) {
		#if ((defined __linux__) || (defined __FreeBSD__) || (defined __APPLE__) || (defined __unix__))
		char buffer[48];
		struct timeval tv;
		gettimeofday( &tv, NULL );
		struct tm tm;
		localtime_r( &tv.tv_sec, &tm );
		snprintf(
			buffer, sizeof( buffer ),
			"%s%02d/%02d %02d:%02d:%02d.%03d%s"
			, level_attributes[level].time_prefix
			, tm.tm_mon + 1, tm.tm_mday
			, tm.tm_hour, tm.tm_min, tm.tm_sec, ( int )( ( ( tv.tv_usec + 500 ) / 1000 ) % 1000 )
			, level_attributes[level].time_suffix
		);
		xlog_payload_append_text( &payload, buffer );
		#else
		#error No implementation for this system.
		#endif
	}
	
	/* package task info */
	if( __xlog_format_been_enabled( module, level, XLOG_FORMAT_OTASK ) ) {
		char taskname[XLOG_LIMIT_THREAD_NAME];
		XLOG_GET_THREAD_NAME( taskname );
		if( taskname[0] == '\0' ) {
			snprintf( taskname, sizeof( taskname ), "%s", "unnamed-task" );
		}
		xlog_payload_append_text_va(
		    &payload, XLOG_TAG_PREFIX_LOG_TASK "%d/%d %s" XLOG_TAG_SUFFIX_LOG_TASK,
		    getppid(), getpid(), taskname
		);
	}
	
	/* package class(level and module path) */
	if( __xlog_format_been_enabled( module, level, XLOG_FORMAT_OLEVEL | XLOG_FORMAT_OMODULE ) ) {
		char modulename[XLOG_LIMIT_MODULE_PATH] = { 0 };
		xlog_payload_append_text( &payload, level_attributes[level].class_prefix );
		if( module && __xlog_format_been_enabled( module, level, XLOG_FORMAT_OMODULE ) ) {
			xlog_payload_append_text( &payload, xlog_module_name( modulename, XLOG_LIMIT_MODULE_PATH, module ) );
		}
		xlog_payload_append_text( &payload, level_attributes[level].class_suffix );
	}
	
	/* package source location */
	if( __xlog_format_been_enabled( module, level, XLOG_FORMAT_OLOCATION ) ) {
		const char *_file = __xlog_format_been_enabled( module, level, XLOG_FORMAT_OFILE ) ? file : NULL;
		const char *_func = __xlog_format_been_enabled( module, level, XLOG_FORMAT_OFUNC ) ? func : NULL;
		long _line = __xlog_format_been_enabled( module, level, XLOG_FORMAT_OLINE ) ? line : -1;
		
		xlog_payload_append_text( &payload, XLOG_TAG_PREFIX_LOG_POINT );
		if( _file ) {
			xlog_payload_append_text( &payload, _file );
		}
		if( _func ) {
			if( _file ) {
				xlog_payload_append_text( &payload, " " );
			}
			xlog_payload_append_text( &payload, _func );
		}
		if( _line != -1 ) {
			if( _file || _func ) {
				xlog_payload_append_text( &payload, ":" );
			}
			char buff[12];
			snprintf( buff, sizeof( buff ), "%ld", _line );
			xlog_payload_append_text( &payload, buff );
		}
		xlog_payload_append_text( &payload, XLOG_TAG_SUFFIX_LOG_POINT );
	}
	
	/* package log body */
	va_list ap;
	va_start( ap, format );
	if( ( context->options & XLOG_CONTEXT_OCOLOR_BODY ) ) {
		xlog_payload_append_text( &payload, level_attributes[level].body_prefix );
	}
	xlog_payload_append_text_va_list( &payload, format, ap );
	if( ( context->options & XLOG_CONTEXT_OCOLOR_BODY ) ) {
		xlog_payload_append_text( &payload, level_attributes[level].body_suffix );
	}
	va_end( ap );
	xlog_payload_append_text( &payload, XLOG_STYLE_NEWLINE );
	if( module ) {
		XLOG_STATS_UPDATE( &module->stats, BYTE, INPUT, payload->offset );
	}
	int length = xlog_payload_print_TEXT( payload, printer );
	xlog_payload_destory( &payload );
	if( length > 0 ) {
		XLOG_STATS_UPDATE( &module->stats, REQUEST, OUTPUT, 1 );
		if( module ) {
			XLOG_STATS_UPDATE( &module->stats, BYTE, OUTPUT, length );
		}
	}
	XLOG_TRACE( "Logged length is %d.", length );
	
	return length;
}
