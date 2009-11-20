/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

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
XBT_INLINE xbt_swag_t xbt_swag_new(size_t offset)
{
  xbt_swag_t swag = xbt_new0(s_xbt_swag_t, 1);

  xbt_swag_init(swag, offset);

  return swag;
}

/**
 * \param swag poor victim
 *
 * kilkil a swag but not it's content. If you do not understand why
 * xbt_swag_free should not free its content, don't use swags.
 */
XBT_INLINE void xbt_swag_free(xbt_swag_t swag)
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
XBT_INLINE void xbt_swag_init(xbt_swag_t swag, size_t offset)
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
 * insert \a obj in \a swag
 */
void xbt_swag_insert(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  xbt_swag_getPrev(obj, swag->offset) = swag->tail;
  xbt_swag_getNext(xbt_swag_getPrev(obj, swag->offset), swag->offset) = obj;

  swag->tail = obj;
}

/**
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the head... you probably had a very good reason to do
 * that, I hope you know what you're doing) \a obj in \a swag
 */
void xbt_swag_insert_at_head(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  xbt_swag_getNext(obj, swag->offset) = swag->head;
  xbt_swag_getPrev(xbt_swag_getNext(obj, swag->offset), swag->offset) = obj;

  swag->head = obj;
}

/**
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the tail... you probably had a very good reason to do
 * that, I hope you know what you're doing) \a obj in \a swag
 */
void xbt_swag_insert_at_tail(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  xbt_swag_getPrev(obj, swag->offset) = swag->tail;
  xbt_swag_getNext(xbt_swag_getPrev(obj, swag->offset), swag->offset) = obj;

  swag->tail = obj;
}

/**
 * \param obj the objet to remove from the swag
 * \param swag a swag
 * \return \a obj if it was in the \a swag and NULL otherwise
 *
 * removes \a obj from \a swag
 */
void *xbt_swag_remove(void *obj, xbt_swag_t swag)
{
  size_t offset = swag->offset;

  if ((!obj) || (!swag))
    return NULL;
  if (!xbt_swag_belongs(obj, swag))     /* Trying to remove an object that
                                           was not in this swag */
    return NULL;

  if (swag->head == swag->tail) {       /* special case */
    if (swag->head != obj)      /* Trying to remove an object that was not in this swag */
      return NULL;
    swag->head = NULL;
    swag->tail = NULL;
    xbt_swag_getNext(obj, offset) = xbt_swag_getPrev(obj, offset) = NULL;
  } else if (obj == swag->head) {       /* It's the head */
    swag->head = xbt_swag_getNext(obj, offset);
    xbt_swag_getPrev(swag->head, offset) = NULL;
    xbt_swag_getNext(obj, offset) = NULL;
  } else if (obj == swag->tail) {       /* It's the tail */
    swag->tail = xbt_swag_getPrev(obj, offset);
    xbt_swag_getNext(swag->tail, offset) = NULL;
    xbt_swag_getPrev(obj, offset) = NULL;
  } else {                      /* It's in the middle */
    xbt_swag_getNext(xbt_swag_getPrev(obj, offset), offset) = xbt_swag_getNext(obj, offset);
    xbt_swag_getPrev(xbt_swag_getNext(obj, offset), offset) = xbt_swag_getPrev(obj, offset);
    xbt_swag_getPrev(obj, offset) = xbt_swag_getNext(obj, offset) = NULL;
  }
  (swag->count)--;
  return obj;
}

/**
 * \param swag a swag
 * \return an object from the \a swag
 */
void *xbt_swag_extract(xbt_swag_t swag)
{
  size_t offset = swag->offset;
  void *obj = NULL;

  if ((!swag) || (!(swag->head)))
    return NULL;

  obj = swag->head;

  if (swag->head == swag->tail) {       /* special case */
    swag->head = swag->tail = NULL;
    xbt_swag_getPrev(obj, offset) = xbt_swag_getNext(obj, offset) = NULL;
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
XBT_INLINE int xbt_swag_size(xbt_swag_t swag)
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

  xbt_test_add0("Basic usage");
  xbt_test_log3("%p %p %ld\n", obj1, &(obj1->setB),
                (long) ((char *) &(obj1->setB) - (char *) obj1));

  setA = xbt_swag_new(xbt_swag_offset(*obj1, setA));
  setB = xbt_swag_new(xbt_swag_offset(*obj1, setB));

  xbt_swag_insert(obj1, setA);
  xbt_swag_insert(obj1, setB);
  xbt_swag_insert(obj2, setA);
  xbt_swag_insert(obj2, setB);

  xbt_swag_remove(obj1, setB);
  /*  xbt_swag_remove(obj2, setB); */

  xbt_test_add0("Traverse set A");
  xbt_swag_foreach(obj, setA) {
    xbt_test_log1("Saw: %s", obj->name);
  }

  xbt_test_add0("Traverse set B");
  xbt_swag_foreach(obj, setB) {
    xbt_test_log1("Saw: %s", obj->name);
  }

  xbt_test_add0("Ensure set content and length");
  xbt_test_assert(xbt_swag_belongs(obj1, setA));
  xbt_test_assert(xbt_swag_belongs(obj2, setA));

  xbt_test_assert(!xbt_swag_belongs(obj1, setB));
  xbt_test_assert(xbt_swag_belongs(obj2, setB));

  xbt_test_assert(xbt_swag_size(setA) == 2);
  xbt_test_assert(xbt_swag_size(setB) == 1);

  xbt_swag_free(setA);
  xbt_swag_free(setB);
}

#endif /* SIMGRID_TEST */
