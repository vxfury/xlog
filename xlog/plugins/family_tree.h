#ifndef __INTERNAL_FAMILY_TREE_H
#define __INTERNAL_FAMILY_TREE_H

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

#define FAMILY_TREE_MALLOC(size)		malloc(size)
#define FAMILY_TREE_REALLOC(ptr, size)	realloc(ptr, size)
#define FAMILY_TREE_FREE(ptr)			free(ptr)

#define FAMILY_TREE_ASSERT(cond)		assert(cond)
#define FAMILY_TREE_TRACE(...)			// fprintf( stderr, "TRACE: <" __FILE__ ":" XSTRING(__LINE__) "> " ), fprintf( stderr, __VA_ARGS__ ), fprintf( stderr, "\r\n" )

typedef struct __family_tree family_tree_t;
struct __family_tree {
	family_tree_t 	*next, *prev;	///< sibling
	family_tree_t 	*child;			///< pointer to first child
	unsigned char    data[0];		///< custom data(variable length)
};

#define FAMILY_TREE_NODE(data)		((family_tree_t *)((char *)(data) - sizeof(family_tree_t)))
#define FAMILY_TREE_DATA(node)		((void *)((char *)(node) + sizeof(family_tree_t)))

#define FAMILY_TREE_PREV(node) 		((node)->prev)
#define FAMILY_TREE_NEXT(node) 		((node)->next)
#define FAMILY_TREE_CHILD(node) 		((node)->child)
#define FAMILY_TREE_PARENT(node) 		(FAMILY_TREE_PREV(node)) /* Valid only when OPRMEM_ISHEAD(mem). */
#define FAMILY_TREE_ISROOT(node) 		(!FAMILY_TREE_PARENT(node))
#define FAMILY_TREE_ISFIRST(node) 	(FAMILY_TREE_NEXT(FAMILY_TREE_PREV(node)) != (node))
#define FAMILY_TREE_ISLAST(node) 		(!FAMILY_TREE_NEXT(node))

/**
 * @brief  Create an family_tree_t
 *
 * @param  size, size of custom data area.
 * @return pointer to `family_tree_t`, NULL if failed to allocate memory.
 *
 */
family_tree_t *family_tree_create( size_t size );

/**
 * @brief  Destory the xlog-tree
 *
 * @param  tree(*), xlog-tree to destory.
 *         hook, called before destory tree node; NULL, do nothing.
 *
 */
void family_tree_destory( family_tree_t *tree, void ( *hook )( family_tree_t * ) );

/**
 * @brief  Resize the family_tree_t
 *
 * @param  node(*), object to resize(SAME AS family_tree_create if `node` is NULL).
 * @param  size, new size of custom data area.
 * @return pointer to `family_tree_t`, NULL if failed to allocate memory.
 *
 */
family_tree_t *family_tree_resize( family_tree_t *node, size_t size );

/**
 * @brief  Get parent of the family_tree_t
 *
 * @param  node, Non-NULL family_tree_t
 * @return pointer to parent of `family_tree_t`, NULL if root node.
 *
 */
family_tree_t *family_tree_parent( const family_tree_t *node );

/**
 * @brief  Set parent of the family_tree_t
 *
 * @param  node(*), Non-NULL family_tree_t
 * @param  parent(*), parent of `node`(NULL if to remove node from old tree)
 *
 */
void family_tree_set_parent( family_tree_t *node, family_tree_t *parent );

/**
 * @brief  Steal node to another tree.
 *
 * @param  node(*), family_tree_t to move.
 * @param  parent(*), destnation parent.
 *
 */
void family_tree_steal( family_tree_t *node, family_tree_t *parent );

/**
 * @brief  Traverse family_tree_t tree
 *
 * @param  node, root node of tree to traverse.
 * @param  func, function to traver node tree.
 * @param  savaptr(*)(Non-NULL), saveptr to avoid access the same node.
 *
 */
void family_tree_traverse( const family_tree_t *node, void ( *func )( const family_tree_t * ), family_tree_t const **saveptr );

#if 0
/**
 * @brief  Get degree of the node
 *
 * @param  node, family_tree_t.
 *
 */
int family_tree_degree( const family_tree_t *node );

/**
 * @brief  Get depth of the tree
 *
 * @param  node, root node of tree.
 *
 */
int family_tree_depth( const family_tree_t *node );

/**
 * @brief  Get children number
 *
 * @param  node, root node of tree.
 *
 */
int family_tree_get_child_number( const family_tree_t *node );

/**
 * @brief  Get sibling number
 *
 * @param  node, family_tree_t.
 *
 */
int family_tree_get_sibling_number( const family_tree_t *node );
#endif

#ifdef __cplusplus
}
#endif

#endif
