#ifndef __COMMON_LIST_H__
#define __COMMON_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "basic_types.h"

#define LIST_POISON1 0
#define LIST_POISON2 0

#define list_entry(ptr, type, member) \
   ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */



#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
   struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
   list->next = list;
   list->prev = list;
}

/*
 * Insert a __new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *__new,
               struct list_head *prev,
               struct list_head *next)
{
   next->prev = __new;
   __new->next = next;
   __new->prev = prev;
   prev->next = __new;
}

/**
 * list_add - add a __new entry
 * @__new: __new entry to be added
 * @head: list head to add it after
 *
 * Insert a __new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *__new, struct list_head *head)
{
   __list_add(__new, head, head->next);
}


/**
 * list_add_tail - add a __new entry
 * @__new: __new entry to be added
 * @head: list head to add it before
 *
 * Insert a __new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *__new, struct list_head *head)
{
   __list_add(__new, head->prev, head);
}

/*
 * Insert a __new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add_rcu(struct list_head * __new,
      struct list_head * prev, struct list_head * next)
{
   __new->next = next;
   __new->prev = prev;
   next->prev = __new;
   prev->next = __new;
}

/**
 * list_add_rcu - add a __new entry to rcu-protected list
 * @__new: __new entry to be added
 * @head: list head to add it after
 *
 * Insert a __new entry after the specified head.
 * This is good for implementing stacks.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_add_rcu()
 * or list_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().
 */
static inline void list_add_rcu(struct list_head *__new, struct list_head *head)
{
   __list_add_rcu(__new, head, head->next);
}

/**
 * list_add_tail_rcu - add a __new entry to rcu-protected list
 * @__new: __new entry to be added
 * @head: list head to add it before
 *
 * Insert a __new entry before the specified head.
 * This is useful for implementing queues.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_add_tail_rcu()
 * or list_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().
 */
static inline void list_add_tail_rcu(struct list_head *__new,
               struct list_head *head)
{
   __list_add_rcu(__new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
   next->prev = prev;
   prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
   __list_del(entry->prev, entry->next);
   entry->next = LIST_POISON1;
   entry->prev = LIST_POISON2;
}

/**
 * list_del_rcu - deletes entry from list without re-initialization
 * @entry: the element to delete from the list.
 *
 * Note: list_empty() on entry does not return true after this,
 * the entry is in an undefined state. It is useful for RCU based
 * lockfree traversal.
 *
 * In particular, it means that we can not poison the forward
 * pointers that may still be used for walking the list.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_del_rcu()
 * or list_add_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().
 *
 * Note that the caller is not permitted to immediately free
 * the __newly deleted entry.  Instead, either synchronize_rcu()
 * or call_rcu() must be used to defer freeing until an RCU
 * grace period has elapsed.
 */
static inline void list_del_rcu(struct list_head *entry)
{
   __list_del(entry->prev, entry->next);
   entry->prev = LIST_POISON2;
}

/**
 * list_replace - replace old entry by __new one
 * @old : the element to be replaced
 * @__new : the __new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head *old,
            struct list_head *__new)
{
   __new->next = old->next;
   __new->next->prev = __new;
   __new->prev = old->prev;
   __new->prev->next = __new;
}

static inline void list_replace_init(struct list_head *old,
               struct list_head *__new)
{
   list_replace(old, __new);
   INIT_LIST_HEAD(old);
}

/**
 * list_replace_rcu - replace old entry by __new one
 * @old : the element to be replaced
 * @__new : the __new element to insert
 *
 * The @old entry will be replaced with the @__new entry atomically.
 * Note: @old should not be empty.
 */
static inline void list_replace_rcu(struct list_head *old,
            struct list_head *__new)
{
   __new->next = old->next;
   __new->prev = old->prev;
   __new->next->prev = __new;
   __new->prev->next = __new;
   old->prev = LIST_POISON2;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
   __list_del(entry->prev, entry->next);
   INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
   __list_del(list->prev, list->next);
   list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list,
              struct list_head *head)
{
   __list_del(list->prev, list->next);
   list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head *list,
            const struct list_head *head)
{
   return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
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
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int list_empty_careful(const struct list_head *head)
{
   struct list_head *next = head->next;
   return (next == head) && (next == head->prev);
}

static inline void __list_splice(struct list_head *list,
             struct list_head *head)
{
   struct list_head *first = list->next;
   struct list_head *last = list->prev;
   struct list_head *at = head->next;

   first->prev = head;
   head->next = first;

   last->next = at;
   at->prev = last;
}

/**
 * list_splice - join two lists
 * @list: the __new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
   if (!list_empty(list))
      __list_splice(list, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the __new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_head *list,
                struct list_head *head)
{
   if (!list_empty(list)) {
      __list_splice(list, head);
      INIT_LIST_HEAD(list);
   }
}

/** 
 * __list_for_each	-	iterate over a list 
 * @pos:	the &struct list_head to use as a loop cursor. 
 * @head:	the head for your list. 
 * 
 * This variant differs from list_for_each() in that it's the 
 * simplest possible list iteration code, no prefetching is done. 
 * Use this for code that knows the list to be very short (empty 
 * or 1 entry) most of the time. 
 */
#define list_for_each(pos, head) \
   for (pos = (head)->next; pos != (head); pos = pos->next)

/** 
 * list_for_each_safe - iterate over a list safe against removal of list entry 
 * @pos: the &struct list_head to use as a loop cursor. 
 * @n:     another &struct list_head to use as temporary storage 
 * @head: the head for your list. 
 */
#define list_for_each_safe(pos, n, head) \
   for (pos = (head)->next, n = pos->next; pos != (head); \
      pos = n, n = pos->next)

#ifdef __cplusplus
}
#endif
#endif
