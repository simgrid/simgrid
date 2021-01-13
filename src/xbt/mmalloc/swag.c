/* Copyright (c) 2004-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */

#include "swag.h"
#include "xbt/asserts.h"

typedef s_xbt_swag_hookup_t *xbt_swag_hookup_t;
typedef struct xbt_swag* xbt_swag_t;
typedef const struct xbt_swag* const_xbt_swag_t;

#define xbt_swag_getPrev(obj, offset) (((xbt_swag_hookup_t)(((char*)(obj)) + (offset)))->prev)
#define xbt_swag_getNext(obj, offset) (((xbt_swag_hookup_t)(((char*)(obj)) + (offset)))->next)
#define xbt_swag_belongs(obj, swag) (xbt_swag_getNext((obj), (swag)->offset) || (swag)->tail == (obj))

static inline void *xbt_swag_getFirst(const_xbt_swag_t swag)
{
  return (swag->head);
}

/*
 * @brief Offset computation
 * @arg var a variable of type <tt>struct</tt> something
 * @arg field a field of <tt>struct</tt> something
 * @return the offset of @a field in <tt>struct</tt> something.
 * @hideinitializer
 *
 * It is very similar to offsetof except that is done at runtime and that you have to declare a variable. Why defining
 * such a macro then ? Because it is portable...
 */
#define xbt_swag_offset(var, field) ((char*)&((var).field) - (char*)&(var))
/* @} */

/* Creates a new swag.
 * @param swag the swag to initialize
 * @param offset where the hookup is located in the structure
 * @see xbt_swag_offset
 *
 * Usage : xbt_swag_init(swag,&obj.setA-&obj);
 */
static inline void xbt_swag_init(xbt_swag_t swag, size_t offset)
{
  swag->tail   = NULL;
  swag->head   = NULL;
  swag->offset = offset;
  swag->count  = 0;
}

/*
 * @param obj the object to insert in the swag
 * @param swag a swag
 *
 * insert (at the tail... you probably had a very good reason to do that, I hope you know what you're doing) @a obj in
 * @a swag
 */
static inline void xbt_swag_insert(void *obj, xbt_swag_t swag)
{
  xbt_assert(!xbt_swag_belongs(obj, swag) || swag->tail,
             "This object belongs to an empty swag! Did you correctly initialize the object's hookup?");

  if (!swag->head) {
    xbt_assert(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    swag->count++;
  } else if (obj != swag->tail && !xbt_swag_getNext(obj, swag->offset)) {
    xbt_swag_getPrev(obj, swag->offset)        = swag->tail;
    xbt_swag_getNext(swag->tail, swag->offset) = obj;
    swag->tail = obj;
    swag->count++;
  }
}

/*
 * @param obj the object to remove from the swag
 * @param swag a swag
 * @return @a obj if it was in the @a swag and NULL otherwise
 *
 * removes @a obj from @a swag
 */
static inline void *xbt_swag_remove(void *obj, xbt_swag_t swag)
{
  if (!obj)
    return NULL;

  size_t offset = swag->offset;
  void* prev    = xbt_swag_getPrev(obj, offset);
  void* next    = xbt_swag_getNext(obj, offset);

  if (prev) {
    xbt_swag_getNext(prev, offset) = next;
    xbt_swag_getPrev(obj, offset)  = NULL;
    if (next) {
      xbt_swag_getPrev(next, offset) = prev;
      xbt_swag_getNext(obj, offset)  = NULL;
    } else {
      swag->tail = prev;
    }
    swag->count--;
  } else if (next) {
    xbt_swag_getPrev(next, offset) = NULL;
    xbt_swag_getNext(obj, offset)  = NULL;
    swag->head = next;
    swag->count--;
  } else if (obj == swag->head) {
    swag->head = swag->tail = NULL;
    swag->count--;
  }

  return obj;
}

/*
 * @param swag a swag
 * @return the number of objects in @a swag
 */
static inline int xbt_swag_size(const_xbt_swag_t swag)
{
  return (swag->count);
}
