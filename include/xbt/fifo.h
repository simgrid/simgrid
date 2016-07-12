/* Copyright (c) 2004-2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_FIFO_H
#define _XBT_FIFO_H
#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h" /* int_f_pvoid_pvoid_t */

SG_BEGIN_DECL()

/** @addtogroup XBT_fifo
 *  @brief This section describes the API to generic workqueue.
 *
 * These functions provide the same kind of functionality as dynamic arrays
 * but in time O(1). However these functions use malloc/free way too much often.
 *
 *  If you are using C++, you might want to used std::list, std::deque or
 *  std::queue instead.
 */
 
/** @defgroup XBT_fifo_cons Fifo constructor and destructor
 *  @ingroup XBT_fifo
 *
 *  @{
 */
/** \brief  Bucket structure 
*/
typedef struct xbt_fifo_item *xbt_fifo_item_t;

/** \brief  FIFO structure
*/
typedef struct xbt_fifo *xbt_fifo_t;

XBT_PUBLIC(xbt_fifo_t) xbt_fifo_new(void);
XBT_PUBLIC(void) xbt_fifo_free(xbt_fifo_t l);
XBT_PUBLIC(void) xbt_fifo_reset(xbt_fifo_t l);
/** @} */

/** @defgroup XBT_fifo_perl Fifo perl-like functions
 *  @ingroup XBT_fifo
 *
 *  @{
 */
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_push(xbt_fifo_t l, void *t);
XBT_PUBLIC(void *) xbt_fifo_pop(xbt_fifo_t l);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_unshift(xbt_fifo_t l, void *t);
XBT_PUBLIC(void *) xbt_fifo_shift(xbt_fifo_t l);
XBT_PUBLIC(int) xbt_fifo_size(xbt_fifo_t l);
XBT_PUBLIC(int) xbt_fifo_is_in(xbt_fifo_t l, void *t);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_search_item(xbt_fifo_t f, int_f_pvoid_pvoid_t cmp_fun, void *closure);
/** @} */

/** @defgroup XBT_fifo_direct Direct access to fifo elements
 *  @ingroup XBT_fifo
 *
 *  @{
 */

XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_new_item(void);
XBT_PUBLIC(void) xbt_fifo_set_item_content(xbt_fifo_item_t i, void *v);
XBT_PUBLIC(void *) xbt_fifo_get_item_content(xbt_fifo_item_t i);
XBT_PUBLIC(void) xbt_fifo_free_item(xbt_fifo_item_t i);

XBT_PUBLIC(void) xbt_fifo_push_item(xbt_fifo_t l, xbt_fifo_item_t i);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_pop_item(xbt_fifo_t i);
XBT_PUBLIC(void) xbt_fifo_unshift_item(xbt_fifo_t l, xbt_fifo_item_t i);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_shift_item(xbt_fifo_t l);

XBT_PUBLIC(int) xbt_fifo_remove(xbt_fifo_t l, void *t);
XBT_PUBLIC(int) xbt_fifo_remove_all(xbt_fifo_t l, void *t);
XBT_PUBLIC(void) xbt_fifo_remove_item(xbt_fifo_t l, xbt_fifo_item_t i);

XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_get_first_item(xbt_fifo_t l);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_get_last_item(xbt_fifo_t l);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_get_next_item(xbt_fifo_item_t i);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_get_prev_item(xbt_fifo_item_t i);

/** 
 * \brief List iterator
 * asserts and stuff
 * \param f a list (#xbt_fifo_t)
 * \param i a bucket (#xbt_fifo_item_t)
 * \param n an object of type \a type.
 * \param type the type of objects contained in the fifo
 * @hideinitializer
 *
 * Iterates over the whole list. 
 */
#define xbt_fifo_foreach(f,i,n,type)                  \
   for(i=xbt_fifo_get_first_item(f);                    \
     ((i)?(n=(type)(xbt_fifo_get_item_content(i)),1):(0));             \
       i=xbt_fifo_get_next_item(i))

/** @} */

/** @defgroup XBT_fifo_misc Misc fifo functions
 *  @ingroup XBT_fifo
 *
 *  @{
 */
XBT_PUBLIC(void **) xbt_fifo_to_array(xbt_fifo_t l);
XBT_PUBLIC(xbt_fifo_t) xbt_fifo_copy(xbt_fifo_t l);
/** @} */

/* Deprecated functions: don't use! */
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_newitem(void);
XBT_PUBLIC(void) xbt_fifo_freeitem(xbt_fifo_item_t l);

XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_getFirstItem(xbt_fifo_t l);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_getNextItem(xbt_fifo_item_t i);
XBT_PUBLIC(xbt_fifo_item_t) xbt_fifo_getPrevItem(xbt_fifo_item_t i);

SG_END_DECL()

#endif                          /* _XBT_FIFO_H */
