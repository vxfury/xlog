#include <xlog/plugins/family_tree.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wpedantic"
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
 * @return pointer to `family_tree_t`, NULL if failed to allocate memory.
 *
 */
family_tree_t *family_tree_create( size_t size )
{
	family_tree_t *tree = ( family_tree_t * )FAMILY_TREE_MALLOC( sizeof( family_tree_t ) + size );
	if( tree ) {
		memset( tree, 0, sizeof( family_tree_t ) + size );
	}
	
	return tree;
}

static inline void __family_tree_destory( family_tree_t *tree, void ( *hook )( family_tree_t * ) )
{
	if( NULL == tree ) {
		return;
	}
	FAMILY_TREE_ASSERT( FAMILY_TREE_PREV( tree ) );
	FAMILY_TREE_PREV( tree ) = NULL;
	
	__family_tree_destory( FAMILY_TREE_CHILD( tree ), hook );
	__family_tree_destory( FAMILY_TREE_NEXT( tree ), hook );
	if( hook ) {
		hook( tree );
	}
	FAMILY_TREE_FREE( tree );
}

/**
 * @brief  Destory the xlog-tree
 *
 * @param  tree(*), xlog-tree to destory.
 *         hook, called before destory tree node; NULL, do nothing.
 *
 */
void family_tree_destory( family_tree_t *tree, void ( *hook )( family_tree_t * ) )
{
	if( tree ) {
		family_tree_set_parent( tree, NULL );
		__family_tree_destory( FAMILY_TREE_CHILD( tree ), hook );
		if( hook ) {
			hook( tree );
		}
		FAMILY_TREE_FREE( tree );
	}
}

/**
 * @brief  Resize the xlog-tree
 *
 * @param  tree(*), object to resize(SAME AS family_tree_create if `tree` is NULL).
 * @param  size, new size of custom data area.
 * @return pointer to `family_tree_t`, NULL if failed to allocate memory.
 *
 */
family_tree_t *family_tree_resize( family_tree_t *tree, size_t size )
{
	family_tree_t *newtree = ( family_tree_t * )FAMILY_TREE_REALLOC( ( void * )tree, sizeof( family_tree_t ) + size );
	if( !tree ) {
		if( newtree ) {
			memset( newtree, 0, sizeof( family_tree_t ) + size );
		}
		return newtree;
	}
	
	if( newtree != tree ) {
		if( FAMILY_TREE_CHILD( newtree ) ) {
			FAMILY_TREE_PARENT( FAMILY_TREE_CHILD( newtree ) ) = newtree;
		}
		
		if( !FAMILY_TREE_ISROOT( newtree ) ) {
			if( FAMILY_TREE_NEXT( newtree ) ) {
				FAMILY_TREE_PREV( FAMILY_TREE_NEXT( newtree ) ) = newtree;
			}
			
			if( FAMILY_TREE_NEXT( FAMILY_TREE_PREV( newtree ) ) == tree ) {
				FAMILY_TREE_NEXT( FAMILY_TREE_PREV( newtree ) ) = newtree;
			}
			
			if( FAMILY_TREE_CHILD( FAMILY_TREE_PARENT( newtree ) ) == tree ) {
				FAMILY_TREE_CHILD( FAMILY_TREE_PARENT( newtree ) ) = newtree;
			}
		}
	}
	return newtree;
}

/**
 * @brief  Get parent of the xlog-tree
 *
 * @param  tree, Non-NULL xlog-tree
 * @return pointer to parent of `family_tree_t`, NULL if root tree.
 *
 */
family_tree_t *family_tree_parent( const family_tree_t *tree )
{
	FAMILY_TREE_ASSERT( tree );
	
	if( FAMILY_TREE_ISROOT( tree ) ) {
		return NULL;
	}
	
	const family_tree_t *tmp = tree;
	while( !FAMILY_TREE_ISFIRST( tmp ) ) {
		tmp = FAMILY_TREE_PREV( tmp );
	}
	
	return FAMILY_TREE_PARENT( tmp );
}

/**
 * @brief  Set parent of the xlog-tree
 *
 * @param  tree(*), Non-NULL xlog-tree
 * @param  parent(*), parent of `tree`(NULL if to remove tree from old tree)
 *
 */
void family_tree_set_parent( family_tree_t *tree, family_tree_t *parent )
{
	FAMILY_TREE_ASSERT( tree );
	
	if( !FAMILY_TREE_ISROOT( tree ) ) {
		FAMILY_TREE_TRACE( "Non-Root, remove tree from old tree." );
		/* Remove tree from old tree */
		if( FAMILY_TREE_NEXT( tree ) ) {
			FAMILY_TREE_TRACE( "Have next tree, link sibling trees." );
			FAMILY_TREE_PREV( FAMILY_TREE_NEXT( tree ) ) = FAMILY_TREE_PREV( tree );
		}
		
		if( !FAMILY_TREE_ISFIRST( tree ) ) {
			FAMILY_TREE_TRACE( "Not the first tree node." );
			FAMILY_TREE_NEXT( FAMILY_TREE_PREV( tree ) ) = FAMILY_TREE_NEXT( tree );
		} else {
			FAMILY_TREE_TRACE( "First tree node. attach next tree to it's parent." );
			FAMILY_TREE_CHILD( FAMILY_TREE_PARENT( tree ) ) = FAMILY_TREE_NEXT( tree );
		}
	}
	
	FAMILY_TREE_TRACE( "Remove relations of this tree" );
	FAMILY_TREE_NEXT( tree ) = FAMILY_TREE_PREV( tree ) = NULL;
	
	if( parent ) {
		/* Insert tree into new tree */
		if( FAMILY_TREE_CHILD( parent ) ) {
			FAMILY_TREE_TRACE( "Child-List is Non-NULL." );
			FAMILY_TREE_NEXT( tree ) = FAMILY_TREE_CHILD( parent );
			FAMILY_TREE_PREV( FAMILY_TREE_CHILD( parent ) ) = tree;
		}
		
		FAMILY_TREE_PARENT( tree ) = parent;
		FAMILY_TREE_CHILD( parent ) = tree;
	}
}

/**
 * @brief  Steal tree to another tree.
 *
 * @param  tree(*), xlog-tree to move.
 * @param  parent(*), destnation parent.
 *
 */
void family_tree_steal( family_tree_t *tree, family_tree_t *parent )
{
	FAMILY_TREE_ASSERT( tree );
	
	family_tree_set_parent( tree, NULL );
	if( !FAMILY_TREE_CHILD( tree ) ) {
		return;
	}
	
	if( parent ) {
		/* Insert tree in front of the list of parent's children. */
		if( FAMILY_TREE_CHILD( parent ) ) {
			family_tree_t *last = FAMILY_TREE_CHILD( tree );
			
			while( FAMILY_TREE_NEXT( last ) ) {
				last = FAMILY_TREE_NEXT( last );
			}
			
			FAMILY_TREE_PREV( FAMILY_TREE_CHILD( parent ) ) = last;
			FAMILY_TREE_NEXT( last ) = FAMILY_TREE_CHILD( parent );
		}
		FAMILY_TREE_CHILD( parent ) = FAMILY_TREE_CHILD( tree );
	}
	FAMILY_TREE_PARENT( FAMILY_TREE_CHILD( tree ) ) = parent;
	FAMILY_TREE_CHILD( tree ) = NULL;
}

/**
 * @brief  Traverse xlog-tree
 *
 * @param  tree, root tree of tree to traverse.
 * @param  func, function to traver tree tree.
 * @param  savaptr(*)(Non-NULL), saveptr to avoid access the same tree.
 *
 */
void family_tree_traverse( const family_tree_t *tree, void ( *func )( const family_tree_t * ), family_tree_t const **saveptr )
{
	FAMILY_TREE_ASSERT( tree );
	FAMILY_TREE_ASSERT( saveptr );
	
	const family_tree_t *cur = tree;
	func( cur );
	if( FAMILY_TREE_CHILD( cur ) ) {
		family_tree_traverse( FAMILY_TREE_CHILD( cur ), func, saveptr );
	}
	
	cur = FAMILY_TREE_NEXT( cur );
	while( cur ) {
		*saveptr = cur;
		family_tree_traverse( cur, func, saveptr );
		cur = FAMILY_TREE_NEXT( *saveptr );
	}
}

#if 0
/**
 * @brief  Get degree of the tree
 *
 * @param  tree, tree node.
 *
 */
int family_tree_degree( const family_tree_t *tree )
{
	int degree = 0;
	const family_tree_t *cur = FAMILY_TREE_CHILD( tree );
	while( cur ) {
		degree ++;
		cur = FAMILY_TREE_NEXT( cur );
	}
	return degree;
}

/**
 * @brief  Get depth of the tree
 *
 * @param  tree, root tree node.
 *
 */
int family_tree_depth( const family_tree_t *tree )
{
	FAMILY_TREE_ASSERT( tree );
	int degree = family_tree_degree( tree );
	int *depths = ( int * )alloca( sizeof( int ) * degree );
	if( !depths ) {
		return 1;
	}
	memset( depths, 0, sizeof( int ) * degree );
	if( !FAMILY_TREE_CHILD( tree ) ) {
		return 1;
	} else {
		const family_tree_t *cur = FAMILY_TREE_CHILD( tree );
		int i = 0;
		while( cur ) {
			depths[i++] += family_tree_depth( cur );
			cur = FAMILY_TREE_NEXT( cur );
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
int family_tree_get_child_number( const family_tree_t *tree )
{
	FAMILY_TREE_ASSERT( tree );
	int rv = 0;
	
	const family_tree_t *cur = FAMILY_TREE_CHILD( tree );
	while( cur ) {
		rv ++;
		rv += family_tree_get_child_number( cur );
		cur = FAMILY_TREE_NEXT( cur );
	}
	
	return rv;
}

/**
 * @brief  Get number of siblings
 *
 * @param  tree, tree node.
 *
 */
int family_tree_get_sibling_number( const family_tree_t *tree )
{
	FAMILY_TREE_ASSERT( tree );
	int rv = 0;
	
	const family_tree_t *cur = tree;
	
	while( !FAMILY_TREE_ISFIRST( cur ) ) {
		rv ++;
		cur = FAMILY_TREE_PREV( cur );
	}
	
	cur = FAMILY_TREE_NEXT( tree );
	while( cur ) {
		rv ++;
		cur = FAMILY_TREE_NEXT( cur );
	}
	
	return rv;
}
#endif
