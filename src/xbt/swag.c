/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */

#include <stdlib.h>
#include <stdio.h>
#include "swag.h"

#define PREV(obj,offset) xbt_swag_getPrev(obj,offset)
#define NEXT(obj,offset) xbt_swag_getNext(obj,offset)

/*
  Usage : xbt_swag_new(&obj.setA-&obj.setA);
 */

xbt_swag_t xbt_swag_new(size_t offset)
{
  xbt_swag_t swag = calloc(1, sizeof(s_xbt_swag_t));

  swag->offset = offset;

  return swag;
}

void xbt_swag_insert(void *obj, xbt_swag_t swag)
{
  (swag->count)++;
  if (swag->head == NULL) {
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  PREV(obj, swag->offset) = swag->tail;
  NEXT(PREV(obj, swag->offset), swag->offset) = obj;

/*   new->prev = l->tail; */
/*   new->prev->next = new; */

  swag->tail = obj;
}

void xbt_swag_extract(void *obj, xbt_swag_t swag)
{
  size_t offset = swag->offset;

  if (swag->head == swag->tail) {	/* special case */
    if (swag->head != obj) {
      fprintf(stderr,
	      "Tried to remove an object that was not in this swag\n");
      abort();
    }
    swag->head = NULL;
    swag->tail = NULL;
    (swag->count)--;
    return;
  }

  if (obj == swag->head) {	/* It's the head */
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
}

int xbt_swag_size(xbt_swag_t swag)
{
  return (swag->count);
}

int xbt_swag_belongs(void *obj, xbt_swag_t swag)
{
  return ((NEXT(obj, swag->offset)) || (PREV(obj, swag->offset)) || (swag->head==obj));
}
