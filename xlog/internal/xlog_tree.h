#ifndef __INTERNAL_XLOG_TREE_H
#define __INTERNAL_XLOG_TREE_H

#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#define XLOG_TREE_MALLOC(size)			malloc(size)
#define XLOG_TREE_REALLOC(ptr, size)	realloc(ptr, size)
#define XLOG_TREE_FREE(ptr)				free(ptr)

#define XLOG_TREE_ASSERT(cond)			assert(cond)
#define XLOG_TREE_TRACE(...)			// do { printf("[XLOG_TREE] <%s:%d> ", __FILE__, __LINE__); printf( __VA_ARGS__ ); printf( "\n" ); } while(0)

typedef struct __xlog_tree xlog_tree_t;
struct __xlog_tree {
	xlog_tree_t 	*next, *prev;	///< sibling
	xlog_tree_t 	*child;			///< pointer to first child
	unsigned char    data[0];		///< custom data(variable length)
};

#define XLOG_TREE_NODE(data)		((xlog_tree_t *)((char *)(data) - sizeof(xlog_tree_t)))
#define XLOG_TREE_DATA(node)		((void *)((char *)(node) + sizeof(xlog_tree_t)))

#define XLOG_TREE_PREV(node) 		((node)->prev)
#define XLOG_TREE_NEXT(node) 		((node)->next)
#define XLOG_TREE_CHILD(node) 		((node)->child)
#define XLOG_TREE_PARENT(node) 		(XLOG_TREE_PREV(node)) /* Valid only when OPRMEM_ISHEAD(mem). */
#define XLOG_TREE_ISROOT(node) 		(!XLOG_TREE_PARENT(node))
#define XLOG_TREE_ISFIRST(node) 	(XLOG_TREE_NEXT(XLOG_TREE_PREV(node)) != (node))
#define XLOG_TREE_ISLAST(node) 		(!XLOG_TREE_NEXT(node))

/**
 * @brief  Create an xlog_tree_t
 *
 * @param  size, size of custom data area.
 * @return pointer to `xlog_tree_t`, NULL if failed to allocate memory.
 *
 */
xlog_tree_t *xlog_tree_create( size_t size );

/**
 * @brief  Destory the xlog_tree_t
 *
 * @param  node(*), xlog_tree_t to destory.
 *
 */
void xlog_tree_destory( xlog_tree_t *node );

/**
 * @brief  Destory the xlog-tree
 *
 * @param  tree(*), xlog-tree to destory.
 *         hook, this function will be executed before destory.
 *
 */
void xlog_tree_destory_with_hook( xlog_tree_t *tree, void (*hook)( xlog_tree_t * ) );

/**
 * @brief  Resize the xlog_tree_t
 *
 * @param  node(*), object to resize(SAME AS xlog_tree_create if `node` is NULL).
 * @param  size, new size of custom data area.
 * @return pointer to `xlog_tree_t`, NULL if failed to allocate memory.
 *
 */
xlog_tree_t *xlog_tree_resize( xlog_tree_t *node, size_t size );

/**
 * @brief  Get parent of the xlog_tree_t
 *
 * @param  node, Non-NULL xlog_tree_t
 * @return pointer to parent of `xlog_tree_t`, NULL if root node.
 *
 */
xlog_tree_t *xlog_tree_parent( const xlog_tree_t *node );

/**
 * @brief  Set parent of the xlog_tree_t
 *
 * @param  node(*), Non-NULL xlog_tree_t
 * @param  parent(*), parent of `node`(NULL if to remove node from old tree)
 *
 */
void xlog_tree_set_parent( xlog_tree_t *node, xlog_tree_t *parent );

/**
 * @brief  Steal node to another tree.
 *
 * @param  node(*), xlog_tree_t to move.
 * @param  parent(*), destnation parent.
 *
 */
void xlog_tree_steal( xlog_tree_t *node, xlog_tree_t *parent );

/**
 * @brief  Traverse xlog_tree_t tree
 *
 * @param  node, root node of tree to traverse.
 * @param  func, function to traver node tree.
 * @param  savaptr(*)(Non-NULL), saveptr to avoid access the same node.
 *
 */
void xlog_tree_traverse( const xlog_tree_t *node, void ( *func )( const xlog_tree_t * ), xlog_tree_t const **saveptr );

/**
 * @brief  Get degree of the node
 *
 * @param  node, xlog_tree_t.
 *
 */
int xlog_tree_degree( const xlog_tree_t *node );

/**
 * @brief  Get depth of the tree
 *
 * @param  node, root node of tree.
 *
 */
int xlog_tree_depth( const xlog_tree_t *node );

/**
 * @brief  Get children number
 *
 * @param  node, root node of tree.
 *
 */
int xlog_tree_get_child_number( const xlog_tree_t *node );

/**
 * @brief  Get sibling number
 *
 * @param  node, xlog_tree_t.
 *
 */
int xlog_tree_get_sibling_number( const xlog_tree_t *node );

#ifdef __cplusplus
}
#endif

#endif
