/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2004-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dynar.h"
#include "simgrid/Exception.hpp"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/string.hpp"
#include "xbt/sysdep.h"
#include <sys/types.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dyn, xbt, "Dynamic arrays");

static inline void _sanity_check_dynar(const_xbt_dynar_t dynar)
{
  xbt_assert(dynar, "dynar is nullptr");
}

static inline void _sanity_check_idx(int idx)
{
  xbt_assert(idx >= 0, "dynar idx(=%d) < 0", idx);
}

static inline void _check_inbound_idx(const_xbt_dynar_t dynar, int idx)
{
  xbt_assert(idx >= 0 && idx < static_cast<int>(dynar->used),
             "dynar is not that long. You asked %d, but it's only %lu long", idx, dynar->used);
}

static inline void _check_populated_dynar(const_xbt_dynar_t dynar)
{
  xbt_assert(dynar->used > 0, "dynar %p is empty", dynar);
}

static inline void _xbt_dynar_resize(xbt_dynar_t dynar, unsigned long new_size)
{
  if (new_size != dynar->size) {
    dynar->size = new_size;
    dynar->data = xbt_realloc(dynar->data, new_size * dynar->elmsize);
  }
}

static inline void _xbt_dynar_expand(xbt_dynar_t dynar, unsigned long nb)
{
  const unsigned long old_size = dynar->size;

  if (nb > old_size) {
    const unsigned long expand = 2 * (old_size + 1);
    _xbt_dynar_resize(dynar, (nb > expand ? nb : expand));
    XBT_DEBUG("expand %p from %lu to %lu elements", dynar, old_size, dynar->size);
  }
}

static inline void* _xbt_dynar_elm(const_xbt_dynar_t dynar, unsigned long idx)
{
  auto* const data            = static_cast<char*>(dynar->data);
  const unsigned long elmsize = dynar->elmsize;

  return data + idx * elmsize;
}

static inline void _xbt_dynar_get_elm(void* dst, const_xbt_dynar_t dynar, unsigned long idx)
{
  const void* const elm = _xbt_dynar_elm(dynar, idx);
  memcpy(dst, elm, dynar->elmsize);
}

/**
 * Creates a new dynar. If a @c free_f is provided, the elements have to be pointer of pointer. That is to say that
 * dynars can contain either base types (int, char, double, etc) or pointer of pointers (struct **).
 *
 * @param elmsize size of each element in the dynar
 * @param free_f function to call each time we want to get rid of an element (or nullptr if nothing to do).
 */
xbt_dynar_t xbt_dynar_new(const unsigned long elmsize, void_f_pvoid_t free_f)
{
  auto* dynar = xbt_new0(s_xbt_dynar_t, 1);

  dynar->size = 0;
  dynar->used = 0;
  dynar->elmsize = elmsize;
  dynar->data = nullptr;
  dynar->free_f = free_f;

  return dynar;
}

/** Destructor of the structure leaving the content unmodified. Ie, the array is freed, but the content is not touched
 * (the @a free_f function is not used).
 *
 * @param dynar poor victim
 */
void xbt_dynar_free_container(xbt_dynar_t* dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_t d = *dynar;
    xbt_free(d->data);
    xbt_free(d);
    *dynar = nullptr;
  }
}

/** @brief Frees the content and set the size to 0 */
void xbt_dynar_reset(xbt_dynar_t dynar)
{
  _sanity_check_dynar(dynar);

  XBT_CDEBUG(xbt_dyn, "Reset the dynar %p", (void *) dynar);
  if (dynar->free_f) {
    xbt_dynar_map(dynar, dynar->free_f);
  }
  dynar->used = 0;
}

/**
 * Shrinks (reduces) the dynar by removing empty slots in the internal storage to save memory.
 * If @c empty_slots_wanted is not zero, this operation preserves that amount of empty slot, for fast future additions.
 * Note that if @c empty_slots_wanted is large enough, the internal array is expanded instead of shrunk.
 *
 * @param dynar a dynar
 * @param empty_slots_wanted number of empty slots elements that can be inserted the internal storage without resizing it
 */
void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots_wanted)
{
  _xbt_dynar_resize(dynar, dynar->used + empty_slots_wanted);
}

/** @brief Destructor: kilkil a dynar and its content. */
void xbt_dynar_free(xbt_dynar_t* dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_reset(*dynar);
    xbt_dynar_free_container(dynar);
  }
}

/** @brief Count of dynar's elements */
unsigned long xbt_dynar_length(const_xbt_dynar_t dynar)
{
  return (dynar ? dynar->used : 0UL);
}

/**@brief check if a dynar is empty */
int xbt_dynar_is_empty(const_xbt_dynar_t dynar)
{
  return (xbt_dynar_length(dynar) == 0);
}

/** @brief Retrieve a copy of the Nth element of a dynar.
 *
 * @param dynar information dealer
 * @param idx index of the slot we want to retrieve
 * @param[out] dst where to put the result to.
 */
void xbt_dynar_get_cpy(const_xbt_dynar_t dynar, unsigned long idx, void* dst)
{
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  _xbt_dynar_get_elm(dst, dynar, idx);
}

/** @brief Retrieve a pointer to the Nth element of a dynar.
 *
 * Note that the returned value is the actual content of the dynar.
 * Make a copy before fooling with it.
 */
void* xbt_dynar_get_ptr(const_xbt_dynar_t dynar, unsigned long idx)
{
  void *res;
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  res = _xbt_dynar_elm(dynar, idx);
  return res;
}

void* xbt_dynar_set_at_ptr(const xbt_dynar_t dynar, unsigned long idx)
{
  _sanity_check_dynar(dynar);

  if (idx >= dynar->used) {
    _xbt_dynar_expand(dynar, idx + 1);
    if (idx > dynar->used) {
      memset(_xbt_dynar_elm(dynar, dynar->used), 0, (idx - dynar->used) * dynar->elmsize);
    }
    dynar->used = idx + 1;
  }
  return _xbt_dynar_elm(dynar, idx);
}

/** @brief Make room for a new element, and return a pointer to it
 *
 * You can then use regular affectation to set its value instead of relying on the slow memcpy. This is what
 * xbt_dynar_insert_at_as() does.
 */
void* xbt_dynar_insert_at_ptr(xbt_dynar_t dynar, int idx)
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
    memmove(_xbt_dynar_elm(dynar, idx + 1), _xbt_dynar_elm(dynar, idx), nb_shift * dynar->elmsize);
  }

  dynar->used = new_used;
  res = _xbt_dynar_elm(dynar, idx);
  return res;
}

/** @brief Set the Nth dynar's element, expanding the dynar and sliding the previous values to the right
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, and moving the previously existing value and all
 * subsequent ones to one position right in the dynar.
 */
void xbt_dynar_insert_at(xbt_dynar_t dynar, int idx, const void* src)
{
  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(xbt_dynar_insert_at_ptr(dynar, idx), src, dynar->elmsize);
}

/** @brief Remove the Nth element, sliding other values to the left
 *
 * Get the Nth element of a dynar, removing it from the dynar and moving all subsequent values to one position left in
 * the dynar.
 *
 * If the object argument of this function is a non-null pointer, the removed element is copied to this address. If not,
 * the element is freed using the free_f function passed at dynar creation.
 */
void xbt_dynar_remove_at(xbt_dynar_t dynar, int idx, void* object)
{
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  if (object) {
    _xbt_dynar_get_elm(object, dynar, idx);
  } else if (dynar->free_f) {
    dynar->free_f(_xbt_dynar_elm(dynar, idx));
  }

  unsigned long nb_shift = dynar->used - 1 - idx;

  if (nb_shift) {
    unsigned long offset = nb_shift * dynar->elmsize;
    memmove(_xbt_dynar_elm(dynar, idx), _xbt_dynar_elm(dynar, idx + 1), offset);
  }

  dynar->used--;
}

/** @brief Returns a boolean indicating whether the element is part of the dynar
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function is probably not
 * what you want. It would compare the pointer values, not the pointed elements.
 */
int xbt_dynar_member(const_xbt_dynar_t dynar, const void* elem)
{
  for (unsigned long it = 0; it < dynar->used; it++)
    if (not memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      return 1;
    }

  return 0;
}

/** @brief Make room at the end of the dynar for a new element, and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying on the slow memcpy. This is what
 * xbt_dynar_push_as() does.
 */
void* xbt_dynar_push_ptr(xbt_dynar_t dynar)
{
  return xbt_dynar_insert_at_ptr(dynar, dynar->used);
}

/** @brief Add an element at the end of the dynar */
void xbt_dynar_push(xbt_dynar_t dynar, const void* src)
{
  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(xbt_dynar_insert_at_ptr(dynar, dynar->used), src, dynar->elmsize);
}

/** @brief Mark the last dynar's element as unused and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying on the slow memcpy. This is what
 * xbt_dynar_pop_as() does.
 */
void* xbt_dynar_pop_ptr(xbt_dynar_t dynar)
{
  _check_populated_dynar(dynar);
  XBT_CDEBUG(xbt_dyn, "Pop %p", (void *) dynar);
  dynar->used--;
  return _xbt_dynar_elm(dynar, dynar->used);
}

/** @brief Get and remove the last element of the dynar */
void xbt_dynar_pop(xbt_dynar_t dynar, void* dst)
{
  /* sanity checks done by remove_at */
  XBT_CDEBUG(xbt_dyn, "Pop %p", (void *) dynar);
  xbt_dynar_remove_at(dynar, dynar->used - 1, dst);
}

/** @brief Add an element at the beginning of the dynar.
 *
 * This is less efficient than xbt_dynar_push()
 */
void xbt_dynar_unshift(xbt_dynar_t dynar, const void* src)
{
  /* sanity checks done by insert_at */
  xbt_dynar_insert_at(dynar, 0, src);
}

/** @brief Get and remove the first element of the dynar.
 *
 * This is less efficient than xbt_dynar_pop()
 */
void xbt_dynar_shift(xbt_dynar_t dynar, void* dst)
{
  /* sanity checks done by remove_at */
  xbt_dynar_remove_at(dynar, 0, dst);
}

/** @brief Apply a function to each member of a dynar
 *
 * The mapped function may change the value of the element itself, but should not mess with the structure of the dynar.
 */
void xbt_dynar_map(const_xbt_dynar_t dynar, void_f_pvoid_t op)
{
  auto* const data            = static_cast<char*>(dynar->data);
  const unsigned long elmsize = dynar->elmsize;
  const unsigned long used = dynar->used;

  _sanity_check_dynar(dynar);

  for (unsigned long i = 0; i < used; i++) {
    char* elm = data + i * elmsize;
    op(elm);
  }
}

/** @brief Sorts a dynar according to the function <tt>compar_fn</tt>
 *
 * This function simply apply the classical qsort(3) function to the data stored in the dynar.
 * You should thus refer to the libc documentation, or to some online tutorial on how to write
 * a comparison function. Here is a quick example if you have integers in your dynar:
 *
 * @verbatim
   int cmpfunc (const void * a, const void * b) {
     int intA = *(int*)a;
     int intB = *(int*)b;
     return intA - intB;
   }
   @endverbatim
 *
 * And now, a function to sort a dynar of MSG hosts depending on their speed:
 * @verbatim
   int cmpfunc(const MSG_host_t a, const MSG_host_t b) {
     MSG_host_t hostA = *(MSG_host_t*)a;
     MSG_host_t hostB = *(MSG_host_t*)b;
     return MSG_host_get_speed(hostA) - MSG_host_get_speed(hostB);
   }
   @endverbatim
 *
 * @param dynar the dynar to sort
 * @param compar_fn comparison function of type (int (compar_fn*) (const void*) (const void*)).
 */
void xbt_dynar_sort(const_xbt_dynar_t dynar, int_f_cpvoid_cpvoid_t compar_fn)
{
  if (dynar->data != nullptr)
    qsort(dynar->data, dynar->used, dynar->elmsize, compar_fn);
}
