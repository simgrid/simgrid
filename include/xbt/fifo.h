/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_FIFO_H
#define _XBT_FIFO_H
#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL()

/** \addtogroup XBT_fifo
 *  @{ */

/** @name 1. Constructor/destructor
 *  @{
 */

/** \brief  Bucket structure 
*/
typedef struct xbt_fifo_item *xbt_fifo_item_t;

/** \brief  FIFO structure
*/
typedef struct xbt_fifo *xbt_fifo_t;

xbt_fifo_t xbt_fifo_new(void);
void xbt_fifo_free(xbt_fifo_t);
/** @} */

/** @name 2. Perl-like functions
 *  @{
 */
xbt_fifo_item_t xbt_fifo_push(xbt_fifo_t, void *);
void *xbt_fifo_pop(xbt_fifo_t);
xbt_fifo_item_t xbt_fifo_unshift(xbt_fifo_t, void *);
void *xbt_fifo_shift(xbt_fifo_t);
int xbt_fifo_size(xbt_fifo_t);
int xbt_fifo_is_in(xbt_fifo_t, void *);
/** @} */

/** @name 3. Manipulating directly items
 *
 *  @{
 */

xbt_fifo_item_t xbt_fifo_newitem(void);
void xbt_fifo_set_item_content(xbt_fifo_item_t, void *);
void *xbt_fifo_get_item_content(xbt_fifo_item_t);
void xbt_fifo_freeitem(xbt_fifo_item_t);

void xbt_fifo_push_item(xbt_fifo_t, xbt_fifo_item_t);
xbt_fifo_item_t xbt_fifo_pop_item(xbt_fifo_t);
void xbt_fifo_unshift_item(xbt_fifo_t, xbt_fifo_item_t);
xbt_fifo_item_t xbt_fifo_shift_item(xbt_fifo_t);

void xbt_fifo_remove(xbt_fifo_t, void *);
void xbt_fifo_remove_item(xbt_fifo_t, xbt_fifo_item_t);

xbt_fifo_item_t xbt_fifo_getFirstItem(xbt_fifo_t l);
xbt_fifo_item_t xbt_fifo_getNextItem(xbt_fifo_item_t i);
xbt_fifo_item_t xbt_fifo_getPrevItem(xbt_fifo_item_t i);

/** 
 * \brief List iterator
 * asserts and stuff
 * \param f a list (#xbt_fifo_t)
 * \param i a bucket (#xbt_fifo_item_t)
 * \param type a type
 * \param n an object of type \a type.
 *
 * Iterates over the whole list. 
 */
#define xbt_fifo_foreach(f,i,n,type)                  \
   for(i=xbt_fifo_getFirstItem(f);                    \
     ((i)?(n=(type)(xbt_fifo_get_item_content(i))):(NULL));             \
       i=xbt_fifo_getNextItem(i))

/** @} */

/** @name 4. Miscanaleous
 *
 *  @{
 */
void **xbt_fifo_to_array(xbt_fifo_t);
xbt_fifo_t xbt_fifo_copy(xbt_fifo_t);
/** @} */

END_DECL()

/** @} */
#endif				/* _XBT_FIFO_H */
