/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_FIFO_H
#define _XBT_FIFO_H

/* Bucket structure */
typedef struct xbt_fifo_item *xbt_fifo_item_t;

/* FIFO structure */
typedef struct xbt_fifo *xbt_fifo_t;

/* API */
xbt_fifo_t xbt_fifo_new(void);
void xbt_fifo_free(xbt_fifo_t);

void xbt_fifo_push(xbt_fifo_t, void *);
void *xbt_fifo_pop(xbt_fifo_t);
void xbt_fifo_unshift(xbt_fifo_t, void *);
void *xbt_fifo_shift(xbt_fifo_t);

void xbt_fifo_remove(xbt_fifo_t, void *);
void xbt_fifo_remove_item(xbt_fifo_t, xbt_fifo_item_t);

int xbt_fifo_is_in(xbt_fifo_t, void *);

void **xbt_fifo_to_array(xbt_fifo_t);
xbt_fifo_t xbt_fifo_copy(xbt_fifo_t);

xbt_fifo_item_t xbt_fifo_newitem(void);
void xbt_fifo_set_item_content(xbt_fifo_item_t, void *);
void *xbt_fifo_get_item_content(xbt_fifo_item_t);
void xbt_fifo_freeitem(xbt_fifo_item_t);

int xbt_fifo_size(xbt_fifo_t);

/* #define xbt_fifo_foreach(f,i,n,type)                  \ */
/*    for(i=xbt_fifo_getFirstitem(f);                    \ */
/*      ((i)?(n=(type)(i->content)):(NULL));             \ */
/*        i=xbt_fifo_getNextitem(i)) */


#endif				/* _XBT_FIFO_H */
