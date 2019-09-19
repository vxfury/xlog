#include <xlog/xlog_config.h>
#include "internal/xlog_tree.h"

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

/**
 * @brief  create a xlog-tree
 *
 * @param  size, size of custom data area.
 * @return pointer to `xlog_tree_t`, NULL if failed to allocate memory.
 *
 */
xlog_tree_t *xlog_tree_create( size_t size )
{
	xlog_tree_t *tree = ( xlog_tree_t * )XLOG_TREE_MALLOC( sizeof( xlog_tree_t ) + size );
	if( tree ) {
		memset( tree, 0, sizeof( xlog_tree_t ) + size );
	}
	
	return tree;
}

static inline void __xlog_tree_destory( xlog_tree_t *tree, void (*hook)( xlog_tree_t * ) )
{
	if( NULL == tree ) {
		return;
	}
	XLOG_TREE_ASSERT( XLOG_TREE_PREV( tree ) );
	XLOG_TREE_PREV( tree ) = NULL;
	
	__xlog_tree_destory( XLOG_TREE_CHILD( tree ), hook );
	__xlog_tree_destory( XLOG_TREE_NEXT( tree ), hook );
	if( hook ) {
		hook( tree );
	}
	XLOG_TREE_FREE( tree );
}

/**
 * @brief  Destory the xlog-tree
 *
 * @param  tree(*), xlog-tree to destory.
 *
 */
void xlog_tree_destory( xlog_tree_t *tree )
{
	if( NULL == tree ) {
		return;
	}
	
	xlog_tree_set_parent( tree, NULL );
	__xlog_tree_destory( XLOG_TREE_CHILD( tree ), NULL );
	XLOG_TREE_FREE( tree );
}

/**
 * @brief  Destory the xlog-tree
 *
 * @param  tree(*), xlog-tree to destory.
 *         hook, this function will be executed before destory.
 *
 */
void xlog_tree_destory_with_hook( xlog_tree_t *tree, void (*hook)( xlog_tree_t * ) )
{
	if( NULL == tree ) {
		return;
	}
	
	xlog_tree_set_parent( tree, NULL );
	__xlog_tree_destory( XLOG_TREE_CHILD( tree ), hook );
	if( hook ) {
		hook( tree );
	}
	XLOG_TREE_FREE( tree );
}

/**
 * @brief  Resize the xlog-tree
 *
 * @param  tree(*), object to resize(SAME AS xlog_tree_create if `tree` is NULL).
 * @param  size, new size of custom data area.
 * @return pointer to `xlog_tree_t`, NULL if failed to allocate memory.
 *
 */
xlog_tree_t *xlog_tree_resize( xlog_tree_t *tree, size_t size )
{
	xlog_tree_t *newtree = ( xlog_tree_t * )XLOG_TREE_REALLOC( ( void * )tree, sizeof( xlog_tree_t ) + size );
	if( !tree ) {
		if( newtree ) {
			memset( newtree, 0, sizeof( xlog_tree_t ) + size );
		}
		return newtree;
	}
	
	if( newtree != tree ) {
		if( XLOG_TREE_CHILD( newtree ) ) {
			XLOG_TREE_PARENT( XLOG_TREE_CHILD( newtree ) ) = newtree;
		}
		
		if( !XLOG_TREE_ISROOT( newtree ) ) {
			if( XLOG_TREE_NEXT( newtree ) ) {
				XLOG_TREE_PREV( XLOG_TREE_NEXT( newtree ) ) = newtree;
			}
			
			if( XLOG_TREE_NEXT( XLOG_TREE_PREV( newtree ) ) == tree ) {
				XLOG_TREE_NEXT( XLOG_TREE_PREV( newtree ) ) = newtree;
			}
			
			if( XLOG_TREE_CHILD( XLOG_TREE_PARENT( newtree ) ) == tree ) {
				XLOG_TREE_CHILD( XLOG_TREE_PARENT( newtree ) ) = newtree;
			}
		}
	}
	return newtree;
}

/**
 * @brief  Get parent of the xlog-tree
 *
 * @param  tree, Non-NULL xlog-tree
 * @return pointer to parent of `xlog_tree_t`, NULL if root tree.
 *
 */
xlog_tree_t *xlog_tree_parent( const xlog_tree_t *tree )
{
	XLOG_TREE_ASSERT( tree );
	
	if( XLOG_TREE_ISROOT( tree ) ) {
		return NULL;
	}
	
	const xlog_tree_t *tmp = tree;
	while( !XLOG_TREE_ISFIRST( tmp ) ) {
		tmp = XLOG_TREE_PREV( tmp );
	}
	
	return XLOG_TREE_PARENT( tmp );
}

/**
 * @brief  Set parent of the xlog-tree
 *
 * @param  tree(*), Non-NULL xlog-tree
 * @param  parent(*), parent of `tree`(NULL if to remove tree from old tree)
 *
 */
void xlog_tree_set_parent( xlog_tree_t *tree, xlog_tree_t *parent )
{
	XLOG_TREE_ASSERT( tree );
	
	if( !XLOG_TREE_ISROOT( tree ) ) {
		XLOG_TREE_TRACE( "Non-Root, remove tree from old tree." );
		/* Remove tree from old tree */
		if( XLOG_TREE_NEXT( tree ) ) {
			XLOG_TREE_TRACE( "Have next tree, link sibling trees." );
			XLOG_TREE_PREV( XLOG_TREE_NEXT( tree ) ) = XLOG_TREE_PREV( tree );
		}
		
		if( !XLOG_TREE_ISFIRST( tree ) ) {
			XLOG_TREE_TRACE( "Not the first tree node." );
			XLOG_TREE_NEXT( XLOG_TREE_PREV( tree ) ) = XLOG_TREE_NEXT( tree );
		} else {
			XLOG_TREE_TRACE( "First tree node. attach next tree to it's parent." );
			XLOG_TREE_CHILD( XLOG_TREE_PARENT( tree ) ) = XLOG_TREE_NEXT( tree );
		}
	}
	
	XLOG_TREE_TRACE( "Remove relations of this tree" );
	XLOG_TREE_NEXT( tree ) = XLOG_TREE_PREV( tree ) = NULL;
	
	if( parent ) {
		/* Insert tree into new tree */
		if( XLOG_TREE_CHILD( parent ) ) {
			XLOG_TREE_TRACE( "Child-List is Non-NULL." );
			XLOG_TREE_NEXT( tree ) = XLOG_TREE_CHILD( parent );
			XLOG_TREE_PREV( XLOG_TREE_CHILD( parent ) ) = tree;
		}
		
		XLOG_TREE_PARENT( tree ) = parent;
		XLOG_TREE_CHILD( parent ) = tree;
	}
}

/**
 * @brief  Steal tree to another tree.
 *
 * @param  tree(*), xlog-tree to move.
 * @param  parent(*), destnation parent.
 *
 */
void xlog_tree_steal( xlog_tree_t *tree, xlog_tree_t *parent )
{
	XLOG_TREE_ASSERT( tree );
	
	xlog_tree_set_parent( tree, NULL );
	if( !XLOG_TREE_CHILD( tree ) ) {
		return;
	}
	
	if( parent ) {
		/* Insert tree in front of the list of parent's children. */
		if( XLOG_TREE_CHILD( parent ) ) {
			xlog_tree_t *last = XLOG_TREE_CHILD( tree );
			
			while( XLOG_TREE_NEXT( last ) ) {
				last = XLOG_TREE_NEXT( last );
			}
			
			XLOG_TREE_PREV( XLOG_TREE_CHILD( parent ) ) = last;
			XLOG_TREE_NEXT( last ) = XLOG_TREE_CHILD( parent );
		}
		XLOG_TREE_CHILD( parent ) = XLOG_TREE_CHILD( tree );
	}
	XLOG_TREE_PARENT( XLOG_TREE_CHILD( tree ) ) = parent;
	XLOG_TREE_CHILD( tree ) = NULL;
}

/**
 * @brief  Traverse xlog-tree
 *
 * @param  tree, root tree of tree to traverse.
 * @param  func, function to traver tree tree.
 * @param  savaptr(*)(Non-NULL), saveptr to avoid access the same tree.
 *
 */
void xlog_tree_traverse( const xlog_tree_t *tree, void ( *func )( const xlog_tree_t * ), xlog_tree_t const **saveptr )
{
	XLOG_TREE_ASSERT( tree );
	XLOG_TREE_ASSERT( saveptr );
	
	const xlog_tree_t *cur = tree;
	func( cur );
	if( XLOG_TREE_CHILD( cur ) ) {
		xlog_tree_traverse( XLOG_TREE_CHILD( cur ), func, saveptr );
	}
	
	cur = XLOG_TREE_NEXT( cur );
	while( cur ) {
		*saveptr = cur;
		xlog_tree_traverse( cur, func, saveptr );
		cur = XLOG_TREE_NEXT( *saveptr );
	}
}

/**
 * @brief  Get degree of the tree
 *
 * @param  tree, tree node.
 *
 */
int xlog_tree_degree( const xlog_tree_t *tree )
{
	int degree = 0;
	const xlog_tree_t *cur = XLOG_TREE_CHILD( tree );
	while( cur ) {
		degree ++;
		cur = XLOG_TREE_NEXT( cur );
	}
	return degree;
}

/**
 * @brief  Get depth of the tree
 *
 * @param  tree, root tree node.
 *
 */
int xlog_tree_depth( const xlog_tree_t *tree )
{
	XLOG_TREE_ASSERT( tree );
	int degree = xlog_tree_degree( tree );
	int *depths = (int *)alloca( sizeof( int ) * degree );
	if( !depths ) {
		return 1;
	}
	memset( depths, 0, sizeof( int ) * degree );
	if( !XLOG_TREE_CHILD( tree ) ) {
		return 1;
	} else {
		const xlog_tree_t *cur = XLOG_TREE_CHILD( tree );
		int i = 0;
		while( cur ) {
			depths[i++] += xlog_tree_depth( cur );
			cur = XLOG_TREE_NEXT( cur );
		}
		
		int max = 0;
		for( i = 0; i < degree; i ++ ) {
			if( depths[i] > max ) {
				max = depths[i];
			}
		}
		return max + 1;
	}
}

/**
 * @brief  Get number of children
 *
 * @param  tree, root tree of tree.
 *
 */
int xlog_tree_get_child_number( const xlog_tree_t *tree )
{
	XLOG_TREE_ASSERT( tree );
	int rv = 0;
	
	const xlog_tree_t *cur = XLOG_TREE_CHILD( tree );
	while( cur ) {
		rv ++;
		rv += xlog_tree_get_child_number( cur );
		cur = XLOG_TREE_NEXT( cur );
	}
	
	return rv;
}

/**
 * @brief  Get number of siblings
 *
 * @param  tree, tree node.
 *
 */
int xlog_tree_get_sibling_number( const xlog_tree_t *tree )
{
	XLOG_TREE_ASSERT( tree );
	int rv = 0;
	
	const xlog_tree_t *cur = tree;
	
	while( !XLOG_TREE_ISFIRST( cur ) ) {
		rv ++;
		cur = XLOG_TREE_PREV( cur );
	}
	
	cur = XLOG_TREE_NEXT( tree );
	while( cur ) {
		rv ++;
		cur = XLOG_TREE_NEXT( cur );
	}
	
	return rv;
}

