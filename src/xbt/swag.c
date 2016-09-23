/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/swag.h"

/** Creates a new swag.
 * \param offset where the hookup is located in the structure
 * \see xbt_swag_offset
 *
 * Usage : xbt_swag_new(&obj.setA-&obj);
 */
inline xbt_swag_t xbt_swag_new(size_t offset)
{
  xbt_swag_t swag = xbt_new0(s_xbt_swag_t, 1);

  xbt_swag_init(swag, offset);

  return swag;
}

/**
 * \param swag poor victim
 *
 * kilkil a swag but not it's content. If you do not understand why xbt_swag_free should not free its content,
 * don't use swags.
 */
inline void xbt_swag_free(xbt_swag_t swag)
{
  free(swag);
}

/** Creates a new swag.
 * \param swag the swag to initialize
 * \param offset where the hookup is located in the structure
 * \see xbt_swag_offset
 *
 * Usage : xbt_swag_init(swag,&obj.setA-&obj);
 */
inline void xbt_swag_init(xbt_swag_t swag, size_t offset)
{
  swag->tail = NULL;
  swag->head = NULL;
  swag->offset = offset;
  swag->count = 0;
}

/**
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the head... you probably had a very good reason to do that, I hope you know what you're doing) \a obj in
 * \a swag
 */
inline void xbt_swag_insert_at_head(void *obj, xbt_swag_t swag)
{
  xbt_assert(!xbt_swag_belongs(obj, swag) || swag->tail,
      "This object belongs to an empty swag! Did you correctly initialize the object's hookup?");

  if (!swag->head) {
    xbt_assert(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    swag->count++;
  } else if (obj != swag->head && !xbt_swag_getPrev(obj, swag->offset)) {
    xbt_swag_getNext(obj, swag->offset) = swag->head;
    xbt_swag_getPrev(swag->head, swag->offset) = obj;
    swag->head = obj;
    swag->count++;
  }
}

/**
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the tail... you probably had a very good reason to do that, I hope you know what you're doing) \a obj in
 * \a swag
 */
inline void xbt_swag_insert_at_tail(void *obj, xbt_swag_t swag)
{
  xbt_assert(!xbt_swag_belongs(obj, swag) || swag->tail,
      "This object belongs to an empty swag! Did you correctly initialize the object's hookup?");

  if (!swag->head) {
    xbt_assert(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    swag->count++;
  } else if (obj != swag->tail && !xbt_swag_getNext(obj, swag->offset)) {
    xbt_swag_getPrev(obj, swag->offset) = swag->tail;
    xbt_swag_getNext(swag->tail, swag->offset) = obj;
    swag->tail = obj;
    swag->count++;
  }
}

/**
 * \param obj the objet to remove from the swag
 * \param swag a swag
 * \return \a obj if it was in the \a swag and NULL otherwise
 *
 * removes \a obj from \a swag
 */
inline void *xbt_swag_remove(void *obj, xbt_swag_t swag)
{
  if (!obj)
    return NULL;

  size_t offset = swag->offset;
  void* prev = xbt_swag_getPrev(obj, offset);
  void* next = xbt_swag_getNext(obj, offset);

  if (prev) {
    xbt_swag_getNext(prev, offset) = next;
    xbt_swag_getPrev(obj, offset) = NULL;
    if (next) {
      xbt_swag_getPrev(next, offset) = prev;
      xbt_swag_getNext(obj, offset) = NULL;
    } else {
      swag->tail = prev;
    }
    swag->count--;
  } else if (next) {
    xbt_swag_getPrev(next, offset) = NULL;
    xbt_swag_getNext(obj, offset) = NULL;
    swag->head = next;
    swag->count--;
  } else if (obj == swag->head) {
    swag->head = swag->tail = NULL;
    swag->count--;
  }

  return obj;
}

/**
 * \param swag a swag
 * \return an object from the \a swag
 */
void *xbt_swag_extract(xbt_swag_t swag)
{
  if (!swag->head)
    return NULL;

  size_t offset = swag->offset;
  void* obj = swag->head;

  if (obj == swag->tail) {       /* special case */
    swag->head = swag->tail = NULL;
  } else {
    swag->head = xbt_swag_getNext(obj, offset);
    xbt_swag_getPrev(swag->head, offset) = NULL;
    xbt_swag_getNext(obj, offset) = NULL;
  }
  (swag->count)--;

  return obj;
}

/**
 * \param swag a swag
 * \return the number of objects in \a swag
 */
inline int xbt_swag_size(xbt_swag_t swag)
{
  return (swag->count);
}

#ifdef SIMGRID_TEST

XBT_TEST_SUITE("swag", "Swag data container");

typedef struct {
  s_xbt_swag_hookup_t setA;
  s_xbt_swag_hookup_t setB;
  const char *name;
} shmurtz, s_shmurtz_t, *shmurtz_t;


XBT_TEST_UNIT("basic", test_swag_basic, "Basic usage")
{
  shmurtz_t obj1, obj2, obj;
  xbt_swag_t setA, setB;

  obj1 = xbt_new0(s_shmurtz_t, 1);
  obj2 = xbt_new0(s_shmurtz_t, 1);

  obj1->name = "Obj 1";
  obj2->name = "Obj 2";

  xbt_test_add("Basic usage");
  xbt_test_log("%p %p %ld\n", obj1, &(obj1->setB), (long) ((char *) &(obj1->setB) - (char *) obj1));

  setA = xbt_swag_new(xbt_swag_offset(*obj1, setA));
  setB = xbt_swag_new(xbt_swag_offset(*obj1, setB));

  xbt_swag_insert(obj1, setA);
  xbt_swag_insert(obj1, setB);
  xbt_swag_insert(obj2, setA);
  xbt_swag_insert(obj2, setB);

  xbt_test_assert(xbt_swag_remove(NULL, setB) == NULL);
  xbt_test_assert(xbt_swag_remove(obj1, setB) == obj1);
  /*  xbt_test_assert(xbt_swag_remove(obj2, setB) == obj2); */

  xbt_test_add("Traverse set A");
  xbt_swag_foreach(obj, setA) {
    xbt_test_log("Saw: %s", obj->name);
  }

  xbt_test_add("Traverse set B");
  xbt_swag_foreach(obj, setB) {
    xbt_test_log("Saw: %s", obj->name);
  }

  xbt_test_add("Ensure set content and length");
  xbt_test_assert(xbt_swag_belongs(obj1, setA));
  xbt_test_assert(xbt_swag_belongs(obj2, setA));

  xbt_test_assert(!xbt_swag_belongs(obj1, setB));
  xbt_test_assert(xbt_swag_belongs(obj2, setB));

  xbt_test_assert(xbt_swag_size(setA) == 2);
  xbt_test_assert(xbt_swag_size(setB) == 1);

  xbt_swag_free(setA);
  xbt_swag_free(setB);

  xbt_free(obj1);
  xbt_free(obj2);
}
#endif                          /* SIMGRID_TEST */
