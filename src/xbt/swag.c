/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "xbt/swag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(swag,xbt,"Swag : O(1) set library");

#define PREV(obj,offset) xbt_swag_getPrev(obj,offset)
#define NEXT(obj,offset) xbt_swag_getNext(obj,offset)

/*
  Usage : xbt_swag_new(&obj.setA-&obj);
 */

xbt_swag_t xbt_swag_new(size_t offset)
{
  xbt_swag_t swag = xbt_new0(s_xbt_swag_t, 1);

  swag->offset = offset;

  return swag;
}


void xbt_swag_free(xbt_swag_t swag)
{
  xbt_free(swag);
}

void xbt_swag_init(xbt_swag_t swag, size_t offset)
{
  swag->offset = offset;
}

void xbt_swag_insert(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  PREV(obj, swag->offset) = swag->tail;
  NEXT(PREV(obj, swag->offset), swag->offset) = obj;

  swag->tail = obj;
}

void *xbt_swag_remove(void *obj, xbt_swag_t swag)
{
  size_t offset = swag->offset;

  if ((!obj) || (!swag))
    return NULL;
  if(!xbt_swag_belongs(obj, swag)) /* Trying to remove an object that
				      was not in this swag */
      return NULL;

  if (swag->head == swag->tail) {	/* special case */
    if (swag->head != obj)	/* Trying to remove an object that was not in this swag */
      return NULL;
    swag->head = NULL;
    swag->tail = NULL;
  } else if (obj == swag->head) {	/* It's the head */
    swag->head = NEXT(obj, offset);
    PREV(swag->head, offset) = NULL;
    NEXT(obj, offset) = NULL;
  } else if (obj == swag->tail) {	/* It's the tail */
    swag->tail = PREV(obj, offset);
    NEXT(swag->tail, offset) = NULL;
    PREV(obj, offset) = NULL;
  } else {			/* It's in the middle */
    NEXT(PREV(obj, offset), offset) = NEXT(obj, offset);
    PREV(NEXT(obj, offset), offset) = PREV(obj, offset);
    PREV(obj, offset) = NEXT(obj, offset) = NULL;
  }
  (swag->count)--;
  return obj;
}

void *xbt_swag_extract(xbt_swag_t swag)
{
  size_t offset = swag->offset;
  void *obj = NULL;

  if ((!swag) || (!(swag->head)))
    return NULL;

  obj = swag->head;

  if (swag->head == swag->tail) {	/* special case */
    swag->head = swag->tail = NULL;
  } else {
    swag->head = NEXT(obj, offset);
    PREV(swag->head, offset) = NULL;
    NEXT(obj, offset) = NULL;
  }
  (swag->count)--;

  return obj;
}

int xbt_swag_size(xbt_swag_t swag)
{
  return (swag->count);
}

int xbt_swag_belongs(void *obj, xbt_swag_t swag)
{
  return ((NEXT(obj, swag->offset)) || (PREV(obj, swag->offset))
	  || (swag->head == obj));
}
