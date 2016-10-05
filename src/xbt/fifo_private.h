/* Copyright (c) 2004, 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_FIFO_PRIVATE_H
#define _XBT_FIFO_PRIVATE_H
#include "xbt/fifo.h"

/* Bucket structure */
typedef struct xbt_fifo_item {
  void *content;
  struct xbt_fifo_item *next;
  struct xbt_fifo_item *prev;
} s_xbt_fifo_item_t;

/* FIFO structure */
typedef struct xbt_fifo {
  int count;
  xbt_fifo_item_t head;
  xbt_fifo_item_t tail;
} s_xbt_fifo_t;

#define xbt_fifo_getFirstitem(l) ((l)?(l)->head:NULL)
#define xbt_fifo_getNextitem(i) ((i)?(i)->next:NULL)
#define xbt_fifo_getPrevitem(i) ((i)?(i)->prev:NULL)
#define xbt_fifo_getItemcontent(i) ((i)?(i)->content:NULL)
#define xbt_fifo_Itemcontent(i) ((i)->content)
#define xbt_fifo_setItemcontent(i,v) (i->content=v)
#endif                          /* _XBT_FIFO_PRIVATE_H */
