/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"           /* SIZEOF_MAX */
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include <sys/types.h>

/* IMPLEMENTATION NOTE ON SYNCHRONIZATION: every functions which name is prefixed by _
 * assumes that the dynar is already locked if we have to.
 * Other functions (public ones) check for this.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dyn, xbt, "Dynamic arrays");

static XBT_INLINE void _dynar_lock(xbt_dynar_t dynar)
{
  if (dynar->mutex)
    xbt_mutex_acquire(dynar->mutex);
}

static XBT_INLINE void _dynar_unlock(xbt_dynar_t dynar)
{
  if (dynar->mutex)
    xbt_mutex_release(dynar->mutex);
}

static XBT_INLINE void _sanity_check_dynar(xbt_dynar_t dynar)
{
  xbt_assert0(dynar, "dynar is NULL");
}

static XBT_INLINE void _sanity_check_idx(int idx)
{
  xbt_assert1(idx >= 0, "dynar idx(=%d) < 0", (int) (idx));
}

static XBT_INLINE void _check_inbound_idx(xbt_dynar_t dynar, int idx)
{
  if (idx < 0 || idx >= dynar->used) {
    _dynar_unlock(dynar);
    THROW2(bound_error, idx,
           "dynar is not that long. You asked %d, but it's only %lu long",
           (int) (idx), (unsigned long) dynar->used);
  }
}

static XBT_INLINE void _check_sloppy_inbound_idx(xbt_dynar_t dynar,
                                                 int idx)
{
  if (idx > dynar->used) {
    _dynar_unlock(dynar);
    THROW2(bound_error, idx,
           "dynar is not that long. You asked %d, but it's only %lu long (could have been equal to it)",
           (int) (idx), (unsigned long) dynar->used);
  }
}

static XBT_INLINE void _check_populated_dynar(xbt_dynar_t dynar)
{
  if (dynar->used == 0) {
    _dynar_unlock(dynar);
    THROW1(bound_error, 0, "dynar %p is empty", dynar);
  }
}

static void _dynar_map(const xbt_dynar_t dynar, void_f_pvoid_t const op);

static XBT_INLINE
    void _xbt_clear_mem(void *const ptr, const unsigned long length)
{
  memset(ptr, 0, length);
}

static XBT_INLINE
    void _xbt_dynar_expand(xbt_dynar_t const dynar, const unsigned long nb)
{
  const unsigned long old_size = dynar->size;

  if (nb > old_size) {
    char *const old_data = (char *) dynar->data;

    const unsigned long elmsize = dynar->elmsize;

    const unsigned long used = dynar->used;
    const unsigned long used_length = used * elmsize;

    const unsigned long new_size =
        nb > (2 * (old_size + 1)) ? nb : (2 * (old_size + 1));
    const unsigned long new_length = new_size * elmsize;
    char *const new_data = (char *) xbt_malloc0(elmsize * new_size);

    DEBUG3("expend %p from %lu to %lu elements", (void *) dynar,
           (unsigned long) old_size, nb);

    if (old_data) {
      memcpy(new_data, old_data, used_length);
      free(old_data);
    }

    _xbt_clear_mem(new_data + used_length, new_length - used_length);

    dynar->size = new_size;
    dynar->data = new_data;
  }
}

static XBT_INLINE
    void *_xbt_dynar_elm(const xbt_dynar_t dynar, const unsigned long idx)
{
  char *const data = (char *) dynar->data;
  const unsigned long elmsize = dynar->elmsize;

  return data + idx * elmsize;
}

static XBT_INLINE
    void
_xbt_dynar_get_elm(void *const dst,
                   const xbt_dynar_t dynar, const unsigned long idx)
{
  void *const elm = _xbt_dynar_elm(dynar, idx);

  memcpy(dst, elm, dynar->elmsize);
}

static XBT_INLINE
    void
_xbt_dynar_put_elm(const xbt_dynar_t dynar,
                   const unsigned long idx, const void *const src)
{
  void *const elm = _xbt_dynar_elm(dynar, idx);
  const unsigned long elmsize = dynar->elmsize;

  memcpy(elm, src, elmsize);
}

static XBT_INLINE
    void
_xbt_dynar_remove_at(xbt_dynar_t const dynar,
                     const unsigned long idx, void *const object)
{

  unsigned long nb_shift;
  unsigned long offset;

  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  if (object) {
    _xbt_dynar_get_elm(object, dynar, idx);
  } else if (dynar->free_f) {
    if (dynar->elmsize <= SIZEOF_MAX) {
      char elm[SIZEOF_MAX];
      _xbt_dynar_get_elm(elm, dynar, idx);
      (*dynar->free_f) (elm);
    } else {
      char *elm = malloc(dynar->elmsize);
      _xbt_dynar_get_elm(elm, dynar, idx);
      (*dynar->free_f) (elm);
      free(elm);
    }
  }

  nb_shift = dynar->used - 1 - idx;

  if (nb_shift) {
    offset = nb_shift * dynar->elmsize;
    memmove(_xbt_dynar_elm(dynar, idx), _xbt_dynar_elm(dynar, idx + 1),
            offset);
  }

  dynar->used--;
}

void xbt_dynar_dump(xbt_dynar_t dynar)
{
  INFO5("Dynar dump: size=%lu; used=%lu; elmsize=%lu; data=%p; free_f=%p",
        dynar->size, dynar->used, dynar->elmsize, dynar->data,
        dynar->free_f);
}

/** @brief Constructor
 *
 * \param elmsize size of each element in the dynar
 * \param free_f function to call each time we want to get rid of an element (or NULL if nothing to do).
 *
 * Creates a new dynar. If a free_func is provided, the elements have to be
 * pointer of pointer. That is to say that dynars can contain either base
 * types (int, char, double, etc) or pointer of pointers (struct **).
 */
xbt_dynar_t
xbt_dynar_new(const unsigned long elmsize, void_f_pvoid_t const free_f)
{

  xbt_dynar_t dynar = xbt_new0(s_xbt_dynar_t, 1);

  dynar->size = 0;
  dynar->used = 0;
  dynar->elmsize = elmsize;
  dynar->data = NULL;
  dynar->free_f = free_f;
  dynar->mutex = NULL;

  return dynar;
}

/** @brief Creates a synchronized dynar.
 *
 * Just like #xbt_dynar_new, but each access to the structure will be protected by a mutex
 *
 */
xbt_dynar_t
xbt_dynar_new_sync(const unsigned long elmsize,
                   void_f_pvoid_t const free_f)
{
  xbt_dynar_t res = xbt_dynar_new(elmsize, free_f);
  res->mutex = xbt_mutex_init();
  return res;
}

/** @brief Destructor of the structure not touching to the content
 *
 * \param dynar poor victim
 *
 * kilkil a dynar BUT NOT its content. Ie, the array is freed, but the content
 * is not touched (the \a free_f function is not used)
 */
void xbt_dynar_free_container(xbt_dynar_t * dynar)
{
  if (dynar && *dynar) {

    if ((*dynar)->data) {
      _xbt_clear_mem((*dynar)->data, (*dynar)->size);
      free((*dynar)->data);
    }

    if ((*dynar)->mutex)
      xbt_mutex_destroy((*dynar)->mutex);

    _xbt_clear_mem(*dynar, sizeof(s_xbt_dynar_t));

    free(*dynar);
    *dynar = NULL;
  }
}

/** @brief Frees the content and set the size to 0
 *
 * \param dynar who to squeeze
 */
XBT_INLINE void xbt_dynar_reset(xbt_dynar_t const dynar)
{
  _dynar_lock(dynar);

  _sanity_check_dynar(dynar);

  DEBUG1("Reset the dynar %p", (void *) dynar);
  if (dynar->free_f) {
    _dynar_map(dynar, dynar->free_f);
  }
  /*
     if (dynar->data)
     free(dynar->data);

     dynar->size = 0;
   */
  dynar->used = 0;

  _dynar_unlock(dynar);

  /*  dynar->data = NULL; */
}

/**
 * \brief Shrink the dynar by removing empty slots at the end of the internal array
 * \param dynar a dynar
 * \param empty_slots_wanted number of empty slots you want to keep at the end of the
 * internal array for further insertions
 *
 * Reduces the internal array size of the dynar to the number of elements plus
 * \a empty_slots_wanted.
 * After removing elements from the dynar, you can call this function to make
 * the dynar use less memory.
 * Set \a empty_slots_wanted to zero to reduce the dynar internal array as much
 * as possible.
 * Note that if \a empty_slots_wanted is greater than the array size, the internal
 * array is expanded instead of shriked.
 */
void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots_wanted)
{
  unsigned long size_wanted;

  _dynar_lock(dynar);

  size_wanted = dynar->used + empty_slots_wanted;
  if (size_wanted != dynar->size) {
    dynar->size = size_wanted;
    dynar->data = xbt_realloc(dynar->data, dynar->elmsize * dynar->size);
  }
  _dynar_unlock(dynar);
}

/** @brief Destructor
 *
 * \param dynar poor victim
 *
 * kilkil a dynar and its content
 */

XBT_INLINE void xbt_dynar_free(xbt_dynar_t * dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_reset(*dynar);
    xbt_dynar_free_container(dynar);
  }
}

/** \brief free a dynar passed as void* (handy to store dynar in dynars or dict) */
void xbt_dynar_free_voidp(void *d)
{
  xbt_dynar_free((xbt_dynar_t *) d);
}

/** @brief Count of dynar's elements
 *
 * \param dynar the dynar we want to mesure
 */
XBT_INLINE unsigned long xbt_dynar_length(const xbt_dynar_t dynar)
{
  return (dynar ? (unsigned long) dynar->used : (unsigned long) 0);
}

/**@brief check if a dynar is empty
 *
 *\param dynar the dynat we want to check
 */

XBT_INLINE int xbt_dynar_is_empty(const xbt_dynar_t dynar)
{
  return (xbt_dynar_length(dynar) == 0);
}

/** @brief Retrieve a copy of the Nth element of a dynar.
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to retrieve
 * \param[out] dst where to put the result to.
 */
XBT_INLINE void
xbt_dynar_get_cpy(const xbt_dynar_t dynar,
                  const unsigned long idx, void *const dst)
{
  _dynar_lock(dynar);
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  _xbt_dynar_get_elm(dst, dynar, idx);
  _dynar_unlock(dynar);
}

/** @brief Retrieve a pointer to the Nth element of a dynar.
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to retrieve
 * \return the \a idx-th element of \a dynar.
 *
 * \warning The returned value is the actual content of the dynar.
 * Make a copy before fooling with it.
 */
XBT_INLINE void *xbt_dynar_get_ptr(const xbt_dynar_t dynar,
                                   const unsigned long idx)
{

  void *res;
  _dynar_lock(dynar);
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  res = _xbt_dynar_elm(dynar, idx);
  _dynar_unlock(dynar);
  return res;
}


static void XBT_INLINE          /* not synchronized */
_xbt_dynar_set(xbt_dynar_t dynar,
               const unsigned long idx, const void *const src)
{

  _sanity_check_dynar(dynar);
  _sanity_check_idx(idx);

  _xbt_dynar_expand(dynar, idx + 1);

  if (idx >= dynar->used) {
    dynar->used = idx + 1;
  }

  _xbt_dynar_put_elm(dynar, idx, src);
}

/** @brief Set the Nth element of a dynar (expended if needed). Previous value at this position is NOT freed
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to modify
 * \param src What will be feeded to the dynar
 *
 * If you want to free the previous content, use xbt_dynar_replace().
 */
XBT_INLINE void xbt_dynar_set(xbt_dynar_t dynar, const int idx,
                              const void *const src)
{

  _dynar_lock(dynar);
  _xbt_dynar_set(dynar, idx, src);
  _dynar_unlock(dynar);
}

/** @brief Set the Nth element of a dynar (expended if needed). Previous value is freed
 *
 * \param dynar
 * \param idx
 * \param object
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, AND DO
 * free the previous value at this position. If you don't want to free the
 * previous content, use xbt_dynar_set().
 */
void
xbt_dynar_replace(xbt_dynar_t dynar,
                  const unsigned long idx, const void *const object)
{
  _dynar_lock(dynar);
  _sanity_check_dynar(dynar);

  if (idx < dynar->used && dynar->free_f) {
    void *const old_object = _xbt_dynar_elm(dynar, idx);

    (*(dynar->free_f)) (old_object);
  }

  _xbt_dynar_set(dynar, idx, object);
  _dynar_unlock(dynar);
}

static XBT_INLINE void *_xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar,
                                                 const unsigned long idx)
{
  void *res;
  unsigned long old_used;
  unsigned long new_used;
  long nb_shift;

  _sanity_check_dynar(dynar);
  _sanity_check_idx(idx);

  old_used = dynar->used;
  new_used = old_used + 1;

  _xbt_dynar_expand(dynar, new_used);

  nb_shift = old_used - idx;

  if (nb_shift>0) {
    memmove(_xbt_dynar_elm(dynar, idx + 1),
            _xbt_dynar_elm(dynar, idx), nb_shift * dynar->elmsize);
  }

  dynar->used = new_used;
  res = _xbt_dynar_elm(dynar, idx);
  return res;
}

/** @brief Make room for a new element, and return a pointer to it
 *
 * You can then use regular affectation to set its value instead of relying
 * on the slow memcpy. This is what xbt_dynar_insert_at_as() does.
 */
void *xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar, const int idx)
{
  void *res;

  _dynar_lock(dynar);
  res = _xbt_dynar_insert_at_ptr(dynar, idx);
  _dynar_unlock(dynar);
  return res;
}

/** @brief Set the Nth dynar's element, expending the dynar and sliding the previous values to the right
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, and
 * moving the previously existing value and all subsequent ones to one
 * position right in the dynar.
 */
XBT_INLINE void
xbt_dynar_insert_at(xbt_dynar_t const dynar,
                    const int idx, const void *const src)
{

  _dynar_lock(dynar);
  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(_xbt_dynar_insert_at_ptr(dynar, idx), src, dynar->elmsize);
  _dynar_unlock(dynar);
}

/** @brief Remove the Nth dynar's element, sliding the previous values to the left
 *
 * Get the Nth element of a dynar, removing it from the dynar and moving
 * all subsequent values to one position left in the dynar.
 *
 * If the object argument of this function is a non-null pointer, the removed
 * element is copied to this address. If not, the element is freed using the
 * free_f function passed at dynar creation.
 */
void
xbt_dynar_remove_at(xbt_dynar_t const dynar,
                    const int idx, void *const object)
{

  _dynar_lock(dynar);
  _xbt_dynar_remove_at(dynar, idx, object);
  _dynar_unlock(dynar);
}

/** @brief Returns the position of the element in the dynar
 *
 * Raises not_found_error if not found.
 */
unsigned int xbt_dynar_search(xbt_dynar_t const dynar, void *const elem)
{
  unsigned long it;

  _dynar_lock(dynar);
  for (it = 0; it < dynar->used; it++)
    if (!memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      _dynar_unlock(dynar);
      return it;
    }

  _dynar_unlock(dynar);
  THROW2(not_found_error, 0, "Element %p not part of dynar %p", elem,
         dynar);
}

/** @brief Returns a boolean indicating whether the element is part of the dynar */
int xbt_dynar_member(xbt_dynar_t const dynar, void *const elem)
{

  xbt_ex_t e;

  TRY {
    xbt_dynar_search(dynar, elem);
  } CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      return 0;
    }
    RETHROW;
  }
  return 1;
}

/** @brief Make room at the end of the dynar for a new element, and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying
 * on the slow memcpy. This is what xbt_dynar_push_as() does.
 */
XBT_INLINE void *xbt_dynar_push_ptr(xbt_dynar_t const dynar)
{
  void *res;

  /* we have to inline xbt_dynar_insert_at_ptr here to make sure that
     dynar->used don't change between reading it and getting the lock
     within xbt_dynar_insert_at_ptr */
  _dynar_lock(dynar);
  res = _xbt_dynar_insert_at_ptr(dynar, dynar->used);
  _dynar_unlock(dynar);
  return res;
}

/** @brief Add an element at the end of the dynar */
XBT_INLINE void xbt_dynar_push(xbt_dynar_t const dynar,
                               const void *const src)
{
  _dynar_lock(dynar);
  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(_xbt_dynar_insert_at_ptr(dynar, dynar->used), src,
         dynar->elmsize);
  _dynar_unlock(dynar);
}

/** @brief Mark the last dynar's element as unused and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying
 * on the slow memcpy. This is what xbt_dynar_pop_as() does.
 */
XBT_INLINE void *xbt_dynar_pop_ptr(xbt_dynar_t const dynar)
{
  void *res;

  _dynar_lock(dynar);
  _check_populated_dynar(dynar);
  DEBUG1("Pop %p", (void *) dynar);
  dynar->used--;
  res = _xbt_dynar_elm(dynar, dynar->used);
  _dynar_unlock(dynar);
  return res;
}

/** @brief Get and remove the last element of the dynar */
XBT_INLINE void xbt_dynar_pop(xbt_dynar_t const dynar, void *const dst)
{

  /* sanity checks done by remove_at */
  DEBUG1("Pop %p", (void *) dynar);
  _dynar_lock(dynar);
  _xbt_dynar_remove_at(dynar, dynar->used - 1, dst);
  _dynar_unlock(dynar);
}

/** @brief Add an element at the begining of the dynar.
 *
 * This is less efficient than xbt_dynar_push()
 */
XBT_INLINE void xbt_dynar_unshift(xbt_dynar_t const dynar,
                                  const void *const src)
{

  /* sanity checks done by insert_at */
  xbt_dynar_insert_at(dynar, 0, src);
}

/** @brief Get and remove the first element of the dynar.
 *
 * This is less efficient than xbt_dynar_pop()
 */
XBT_INLINE void xbt_dynar_shift(xbt_dynar_t const dynar, void *const dst)
{

  /* sanity checks done by remove_at */
  xbt_dynar_remove_at(dynar, 0, dst);
}

static void _dynar_map(const xbt_dynar_t dynar, void_f_pvoid_t const op)
{
  char elm[SIZEOF_MAX];
  const unsigned long used = dynar->used;
  unsigned long i = 0;

  for (i = 0; i < used; i++) {
    _xbt_dynar_get_elm(elm, dynar, i);
    (*op) (elm);
  }
}

/** @brief Apply a function to each member of a dynar
 *
 * The mapped function may change the value of the element itself,
 * but should not mess with the structure of the dynar.
 *
 * If the dynar is synchronized, it is locked during the whole map
 * operation, so make sure your function don't call any function
 * from xbt_dynar_* on it, or you'll get a deadlock.
 */
XBT_INLINE void xbt_dynar_map(const xbt_dynar_t dynar,
                              void_f_pvoid_t const op)
{

  _sanity_check_dynar(dynar);
  _dynar_lock(dynar);

  _dynar_map(dynar, op);

  _dynar_unlock(dynar);
}


/** @brief Removes and free the entry pointed by the cursor
 *
 * This function can be used while traversing without problem.
 */
XBT_INLINE void xbt_dynar_cursor_rm(xbt_dynar_t dynar,
                                    unsigned int *const cursor)
{

  _xbt_dynar_remove_at(dynar, (*cursor)--, NULL);
}

/** @brief Unlocks a synchronized dynar when you want to break the traversal
 *
 * This function must be used if you <tt>break</tt> the
 * xbt_dynar_foreach loop, but shouldn't be called at the end of a
 * regular traversal reaching the end of the elements
 */
XBT_INLINE void xbt_dynar_cursor_unlock(xbt_dynar_t dynar)
{
  _dynar_unlock(dynar);
}

/** @brief Sorts a dynar according to the function <tt>compar_fn</tt>
 *
 * \param compar_fn comparison function of type (int (compar_fn*) (void*) (void*)).
 *
 * Remark: if the elements stored in the dynar are structures, the compar_fn
 * function has to retrieve the field to sort first.
 */
XBT_INLINE void xbt_dynar_sort(xbt_dynar_t dynar,
                               int_f_cpvoid_cpvoid_t compar_fn)
{

  _dynar_lock(dynar);

  qsort(dynar->data, dynar->used, dynar->elmsize, compar_fn);

  _dynar_unlock(dynar);
}

/*
 * Return 0 if d1 and d2 are equal and 1 if not equal
 */
XBT_INLINE int xbt_dynar_compare(xbt_dynar_t d1, xbt_dynar_t d2,
					int(*compar)(const void *, const void *))
{
	int i ;
	int size;
	if((!d1) && (!d2)) return 0;
	if((!d1) || (!d2))
	{
		DEBUG2("NULL dynar d1=%p d2=%p",d1,d2);
		return 1;
	}
	if((d1->elmsize)!=(d2->elmsize))
	{
		DEBUG2("Size of elmsize d1=%ld d2=%ld",d1->elmsize,d2->elmsize);
		return 1; // xbt_die
	}
	if(xbt_dynar_length(d1) != xbt_dynar_length(d2))
	{
		DEBUG2("Size of dynar d1=%ld d2=%ld",xbt_dynar_length(d1),xbt_dynar_length(d2));
		return 1;
	}

	size = xbt_dynar_length(d1);
	for(i=0;i<size;i++)
	{
		void *data1 = xbt_dynar_get_as(d1, i, void *);
		void *data2 = xbt_dynar_get_as(d2, i, void *);
		DEBUG3("link[%d] d1=%p d2=%p",i,data1,data2);
		if(compar(data1,data2)) return 1;
	}
	return 0;
}

#ifdef SIMGRID_TEST

#define NB_ELEM 5000

XBT_TEST_SUITE("dynar", "Dynar data container");
XBT_LOG_EXTERNAL_CATEGORY(xbt_dyn);
XBT_LOG_DEFAULT_CATEGORY(xbt_dyn);

XBT_TEST_UNIT("int", test_dynar_int, "Dynars of integers")
{
  /* Vars_decl [doxygen cruft] */
  xbt_dynar_t d;
  int i, cpt;
  unsigned int cursor;
  int *iptr;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, i) {
    xbt_assert0(0, "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1
      ("==== Push %d int, set them again 3 times, traverse them, shift them",
       NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);     /* This is faster (and possible only with scalars) */
    /* xbt_dynar_push(d,&cpt);       This would also work */
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 2. Traverse manually the dynar */
  for (cursor = 0; cursor < NB_ELEM; cursor++) {
    iptr = xbt_dynar_get_ptr(d, cursor);
    xbt_test_assert2(cursor == *iptr,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }

  /* 3. Traverse the dynar using the neat macro to that extend */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;
  /*     xbt_dynar_set(d,cpt,&cpt); */

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;

  cpt = 0;
  xbt_dynar_foreach(d, cursor, i) {
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    cpt++;
  }
  xbt_test_assert2(cpt == NB_ELEM,
                   "Cannot retrieve my %d values. Last got one is %d",
                   NB_ELEM, cpt);

  /* shifting [doxygen cruft] */
  /* 4. Shift all the values */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    xbt_test_log2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 5. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Unshift/pop %d int", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_unshift(d, &cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    i = xbt_dynar_pop_as(d, int);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    xbt_test_log2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add1
      ("==== Push %d int, insert 1000 int in the middle, shift everything",
       NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM/5; cpt++) {
    xbt_dynar_insert_at_as(d, NB_ELEM/2, int, cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  for (cpt = 0; cpt < NB_ELEM/2; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
                     i, cpt);
    DEBUG2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
                     i, cpt);
  }
  for (cpt = 2500; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one at the end (%d!=%d)",
                     i, cpt);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Push %d int, remove 2000-4000. free the rest",
                NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_push_as(d, int, cpt);

  for (cpt = 2000; cpt < 4000; cpt++) {
    xbt_dynar_remove_at(d, 2000, &i);
    xbt_test_assert2(i == cpt,
                     "Remove a bad value. Got %d, expected %d", i, cpt);
    DEBUG2("remove %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("insert",test_dynar_insert,"Using the xbt_dynar_insert and xbt_dynar_remove functions")
{
  xbt_dynar_t d = xbt_dynar_new(sizeof(int), NULL);
  unsigned int cursor;
  int cpt;

  xbt_test_add1("==== Insert %d int, traverse them, remove them",NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_insert_at(d, cpt, &cpt);
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  /* Re-fill with the same values using set_as (and re-verify) */
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_set_as(d, cpt, int, cpt);
  xbt_dynar_foreach(d, cursor, cpt)
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);

  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    int val;
    xbt_dynar_remove_at(d,0,&val);
    xbt_test_assert2(cpt == val,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  xbt_test_assert1(xbt_dynar_length(d) == 0,
                   "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);

  /* ********************* */
  xbt_test_add1("==== Insert %d int in reverse order, traverse them, remove them",NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = NB_ELEM-1; cpt >=0; cpt--) {
    xbt_dynar_replace(d, cpt, &cpt);
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  for (cpt =NB_ELEM-1; cpt >=0; cpt--) {
    int val;
    xbt_dynar_remove_at(d,xbt_dynar_length(d)-1,&val);
    xbt_test_assert2(cpt == val,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  xbt_test_assert1(xbt_dynar_length(d) == 0,
                   "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);
}

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("double", test_dynar_double, "Dynars of doubles")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int cursor;
  double d1, d2;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert0(FALSE,
                     "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add0("==== Push/shift 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  xbt_dynar_foreach(d, cursor, d2) {
    d1 = (double) cursor;
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add0("==== Unshift/pop 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_unshift(d, &d1);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_pop(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */



  xbt_test_add0
      ("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  for (cpt = 0; cpt < 1000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_insert_at(d, 2500, &d1);
  }

  for (cpt = 0; cpt < 2500; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one at the begining (%f!=%f)",
                     d1, d2);
    DEBUG2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one in the middle (%f!=%f)",
                     d1, d2);
  }
  for (cpt = 2500; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one at the end (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add0("==== Push 5000 double, remove 2000-4000. free the rest");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  for (cpt = 2000; cpt < 4000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_remove_at(d, 2000, &d2);
    xbt_test_assert2(d1 == d2,
                     "Remove a bad value. Got %f, expected %f", d2, d1);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}


/* doxygen_string_cruft */

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("string", test_dynar_string, "Dynars of strings")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int iter;
  char buf[1024];
  char *s1, *s2;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  xbt_dynar_foreach(d, iter, s1) {
    xbt_test_assert0(FALSE,
                     "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Push %d strings, set them again 3 times, shift them",
                NB_ELEM);
  /* Populate_str [doxygen cruft] */
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Unshift, traverse and pop %d strings", NB_ELEM);
  d = xbt_dynar_new(sizeof(char **), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_unshift(d, &s1);
  }
  /* 2. Traverse the dynar with the macro */
  xbt_dynar_foreach(d, iter, s1) {
    sprintf(buf, "%d", NB_ELEM - iter - 1);
    xbt_test_assert2(!strcmp(buf, s1),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s1);
  }
  /* 3. Traverse the dynar with the macro */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_pop(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  /* 4. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add2
      ("==== Push %d strings, insert %d strings in the middle, shift everything",
       NB_ELEM, NB_ELEM / 5);
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM / 5; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_insert_at(d, NB_ELEM / 2, &s1);
  }

  for (cpt = 0; cpt < NB_ELEM / 2; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one at the begining (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  for (cpt = (NB_ELEM / 5) - 1; cpt >= 0; cpt--) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one in the middle (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  for (cpt = NB_ELEM / 2; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one at the end (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add3("==== Push %d strings, remove %d-%d. free the rest",
                NB_ELEM, 2 * (NB_ELEM / 5), 4 * (NB_ELEM / 5));
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 2 * (NB_ELEM / 5); cpt < 4 * (NB_ELEM / 5); cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_remove_at(d, 2 * (NB_ELEM / 5), &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "Remove a bad value. Got %s, expected %s", s2, buf);
    free(s2);
  }
  xbt_dynar_free(&d);           /* end_of_doxygen */
}


/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
#include "xbt/synchro.h"
static void pusher_f(void *a)
{
  xbt_dynar_t d = (xbt_dynar_t) a;
  int i;
  for (i = 0; i < 500; i++) {
    xbt_dynar_push(d, &i);
  }
}

static void poper_f(void *a)
{
  xbt_dynar_t d = (xbt_dynar_t) a;
  int i;
  int data;
  xbt_ex_t e;

  for (i = 0; i < 500; i++) {
    TRY {
      xbt_dynar_pop(d, &data);
    }
    CATCH(e) {
      if (e.category == bound_error) {
        xbt_ex_free(e);
        i--;
      } else {
        RETHROW;
      }
    }
  }
}


XBT_TEST_UNIT("synchronized int", test_dynar_sync_int, "Synchronized dynars of integers")
{
  /* Vars_decl [doxygen cruft] */
  xbt_dynar_t d;
  xbt_thread_t pusher, poper;

  xbt_test_add0("==== Have a pusher and a popper on the dynar");
  d = xbt_dynar_new_sync(sizeof(int), NULL);
  pusher = xbt_thread_create("pusher", pusher_f, d, 0 /*not joinable */ );
  poper = xbt_thread_create("poper", poper_f, d, 0 /*not joinable */ );
  xbt_thread_join(pusher);
  xbt_thread_join(poper);
  xbt_dynar_free(&d);
}

#endif                          /* SIMGRID_TEST */
