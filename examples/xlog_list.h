#ifndef __XLOG_LIST_H
#define __XLOG_LIST_H

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#define WRITE_ONCE(var, val) (*((volatile typeof(val) *)(&(var))) = (val))
#else
#define WRITE_ONCE(var, val) (var = (val))
#endif

#define container_of(ptr, type, member) ({					\
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );		\
	})
// #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * Required struct contains `next` & `prev`
 */

struct xlog_list_tag;
typedef struct xlog_list_tag xlog_list_t;

static inline void xlog_list_init( xlog_list_t *head )
{
	head->next = head;
	head->prev = head;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __xlog_list_add(
    xlog_list_t *entry,
    xlog_list_t *prev, xlog_list_t *next
)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __xlog_list_del(
    xlog_list_t *prev, xlog_list_t *next
)
{
	next->prev = prev;
	WRITE_ONCE( prev->next, next );
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void xlog_list_add( xlog_list_t *entry, xlog_list_t *head )
{
	__xlog_list_add( entry, head, head->next );
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void xlog_list_add_tail( xlog_list_t *entry, xlog_list_t *head )
{
	__xlog_list_add( entry, head->prev, head );
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __xlog_list_del_entry( xlog_list_t *entry )
{
	__xlog_list_del( entry->prev, entry->next );
}

static inline void xlog_list_del( xlog_list_t *entry )
{
	__xlog_list_del( entry->prev, entry->next );
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * xlog_list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @entry : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void xlog_list_replace(
    xlog_list_t *old, xlog_list_t *entry
)
{
	entry->next = old->next;
	entry->next->prev = entry;
	entry->prev = old->prev;
	entry->prev->next = entry;
}

static inline void xlog_list_replace_init(
    xlog_list_t *old, xlog_list_t *entry
)
{
	xlog_list_replace( old, entry );
	xlog_list_init( old );
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void xlog_list_del_init( xlog_list_t *entry )
{
	__xlog_list_del_entry( entry );
	xlog_list_init( entry );
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void xlog_list_move( xlog_list_t *list, xlog_list_t *head )
{
	__xlog_list_del_entry( list );
	xlog_list_add( list, head );
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void xlog_list_move_tail( xlog_list_t *list,
    xlog_list_t *head )
{
	__xlog_list_del_entry( list );
	xlog_list_add_tail( list, head );
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int xlog_list_is_last( const xlog_list_t *list,
    const xlog_list_t *head )
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int xlog_list_empty( const xlog_list_t *head )
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is xlog_list_del_init(). Eg. it cannot be used
 * if another CPU could re-xlog_list_add() it.
 */
static inline int xlog_list_empty_careful( const xlog_list_t *head )
{
	xlog_list_t *next = head->next;
	return ( next == head ) && ( next == head->prev );
}

/**
 * xlog_list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void xlog_list_rotate_left( xlog_list_t *head )
{
	xlog_list_t *first;
	
	if ( !xlog_list_empty( head ) ) {
		first = head->next;
		xlog_list_move_tail( first, head );
	}
}

/**
 * xlog_list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int xlog_list_is_singular( const xlog_list_t *head )
{
	return !xlog_list_empty( head ) && ( head->next == head->prev );
}

static inline void __xlog_list_cut_position( xlog_list_t *list,
    xlog_list_t *head, xlog_list_t *entry )
{
	xlog_list_t *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * xlog_list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void xlog_list_cut_position( xlog_list_t *list,
    xlog_list_t *head, xlog_list_t *entry )
{
	if ( xlog_list_empty( head ) ) {
		return;
	}
	if ( xlog_list_is_singular( head ) &&
	    ( head->next != entry && head != entry ) ) {
		return;
	}
	if ( entry == head ) {
		xlog_list_init( list );
	} else {
		__xlog_list_cut_position( list, head, entry );
	}
}

static inline void __xlog_list_splice( const xlog_list_t *list,
    xlog_list_t *prev,
    xlog_list_t *next )
{
	xlog_list_t *first = list->next;
	xlog_list_t *last = list->prev;
	
	first->prev = prev;
	prev->next = first;
	
	last->next = next;
	next->prev = last;
}

/**
 * xlog_list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void xlog_list_splice( const xlog_list_t *list,
    xlog_list_t *head )
{
	if ( !xlog_list_empty( list ) ) {
		__xlog_list_splice( list, head, head->next );
	}
}

/**
 * xlog_list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void xlog_list_splice_tail( xlog_list_t *list,
    xlog_list_t *head )
{
	if ( !xlog_list_empty( list ) ) {
		__xlog_list_splice( list, head->prev, head );
	}
}

/**
 * xlog_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void xlog_list_splice_init( xlog_list_t *list,
    xlog_list_t *head )
{
	if ( !xlog_list_empty( list ) ) {
		__xlog_list_splice( list, head, head->next );
		xlog_list_init( list );
	}
}

/**
 * xlog_list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void xlog_list_splice_tail_init( xlog_list_t *list,
    xlog_list_t *head )
{
	if ( !xlog_list_empty( list ) ) {
		__xlog_list_splice( list, head->prev, head );
		xlog_list_init( list );
	}
}

/**
 * xlog_list_entry - get the struct for this entry
 * @ptr:	the &xlog_list_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * xlog_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define xlog_list_first_entry(ptr, type, member) \
	xlog_list_entry((ptr)->next, type, member)

/**
 * xlog_list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define xlog_list_last_entry(ptr, type, member) \
	xlog_list_entry((ptr)->prev, type, member)

/**
 * xlog_list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define xlog_list_first_entry_or_null(ptr, type, member) \
	(!xlog_list_empty(ptr) ? xlog_list_first_entry(ptr, type, member) : NULL)

/**
 * xlog_list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_next_entry(pos, member) \
	xlog_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * xlog_list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_prev_entry(pos, member) \
	xlog_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * xlog_list_for_each	-	iterate over a list
 * @pos:	the &xlog_list_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define xlog_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * xlog_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &xlog_list_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define xlog_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * xlog_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &xlog_list_t to use as a loop cursor.
 * @n:		another &xlog_list_t to use as temporary storage
 * @head:	the head for your list.
 */
#define xlog_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	    pos = n, n = pos->next)

/**
 * xlog_list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &xlog_list_t to use as a loop cursor.
 * @n:		another &xlog_list_t to use as temporary storage
 * @head:	the head for your list.
 */
#define xlog_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	    pos != (head); \
	    pos = n, n = pos->prev)

/**
 * xlog_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_for_each_entry(pos, head, member)				\
	for (pos = xlog_list_first_entry(head, typeof(*pos), member);	\
	    &pos->member != (head);					\
	    pos = xlog_list_next_entry(pos, member))

/**
 * xlog_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = xlog_list_last_entry(head, typeof(*pos), member);		\
	    &pos->member != (head); 					\
	    pos = xlog_list_prev_entry(pos, member))

/**
 * xlog_list_prepare_entry - prepare a pos entry for use in xlog_list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in xlog_list_for_each_entry_continue().
 */
#define xlog_list_prepare_entry(pos, head, member) \
	((pos) ? : xlog_list_entry(head, typeof(*pos), member))

/**
 * xlog_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define xlog_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = xlog_list_next_entry(pos, member);			\
	    &pos->member != (head);					\
	    pos = xlog_list_next_entry(pos, member))

/**
 * xlog_list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define xlog_list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = xlog_list_prev_entry(pos, member);			\
	    &pos->member != (head);					\
	    pos = xlog_list_prev_entry(pos, member))

/**
 * xlog_list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define xlog_list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	    pos = xlog_list_next_entry(pos, member))

/**
 * xlog_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 */
#define xlog_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = xlog_list_first_entry(head, typeof(*pos), member),	\
	    n = xlog_list_next_entry(pos, member);			\
	    &pos->member != (head); 					\
	    pos = n, n = xlog_list_next_entry(n, member))

/**
 * xlog_list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define xlog_list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = xlog_list_next_entry(pos, member), 				\
	    n = xlog_list_next_entry(pos, member);				\
	    &pos->member != (head);						\
	    pos = n, n = xlog_list_next_entry(n, member))

/**
 * xlog_list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define xlog_list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = xlog_list_next_entry(pos, member);					\
	    &pos->member != (head);						\
	    pos = n, n = xlog_list_next_entry(n, member))

/**
 * xlog_list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the xlog_list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define xlog_list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = xlog_list_last_entry(head, typeof(*pos), member),		\
	    n = xlog_list_prev_entry(pos, member);			\
	    &pos->member != (head); 					\
	    pos = n, n = xlog_list_prev_entry(n, member))

/**
 * xlog_list_safe_reset_next - reset a stale xlog_list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the xlog_list_for_each_entry_safe loop
 * @n:		temporary storage used in xlog_list_for_each_entry_safe
 * @member:	the name of the xlog_list_head within the struct.
 *
 * xlog_list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and xlog_list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define xlog_list_safe_reset_next(pos, n, member)				\
	n = xlog_list_next_entry(pos, member)


#endif
