/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2004-2020. The SimGrid Team.
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
  if (idx < 0 || idx >= static_cast<int>(dynar->used)) {
    throw std::out_of_range(simgrid::xbt::string_printf("dynar is not that long. You asked %d, but it's only %lu long",
                                                        idx, static_cast<unsigned long>(dynar->used)));
  }
}

static inline void _check_populated_dynar(const_xbt_dynar_t dynar)
{
  if (dynar->used == 0) {
    throw std::out_of_range(simgrid::xbt::string_printf("dynar %p is empty", dynar));
  }
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
  char *const data = (char *) dynar->data;
  const unsigned long elmsize = dynar->elmsize;

  return data + idx * elmsize;
}

static inline void _xbt_dynar_get_elm(void* dst, const_xbt_dynar_t dynar, unsigned long idx)
{
  const void* const elm = _xbt_dynar_elm(dynar, idx);
  memcpy(dst, elm, dynar->elmsize);
}

void xbt_dynar_dump(const_xbt_dynar_t dynar)
{
  XBT_INFO("Dynar dump: size=%lu; used=%lu; elmsize=%lu; data=%p; free_f=%p",
        dynar->size, dynar->used, dynar->elmsize, dynar->data, dynar->free_f);
}

/** @brief Constructor
 *
 * @param elmsize size of each element in the dynar
 * @param free_f function to call each time we want to get rid of an element (or nullptr if nothing to do).
 *
 * Creates a new dynar. If a free_func is provided, the elements have to be pointer of pointer. That is to say that
 * dynars can contain either base types (int, char, double, etc) or pointer of pointers (struct **).
 */
xbt_dynar_t xbt_dynar_new(const unsigned long elmsize, void_f_pvoid_t free_f)
{
  xbt_dynar_t dynar = xbt_new0(s_xbt_dynar_t, 1);

  dynar->size = 0;
  dynar->used = 0;
  dynar->elmsize = elmsize;
  dynar->data = nullptr;
  dynar->free_f = free_f;

  return dynar;
}

/** @brief Initialize a dynar structure that was not malloc'ed
 * This can be useful to keep temporary dynars on the stack
 */
void xbt_dynar_init(xbt_dynar_t dynar, unsigned long elmsize, void_f_pvoid_t free_f)
{
  dynar->size    = 0;
  dynar->used    = 0;
  dynar->elmsize = elmsize;
  dynar->data    = nullptr;
  dynar->free_f  = free_f;
}

/** @brief Destroy a dynar that was created with xbt_dynar_init */
void xbt_dynar_free_data(xbt_dynar_t dynar)
{
  xbt_dynar_reset(dynar);
  if (dynar)
    xbt_free(dynar->data);
}

/** @brief Destructor of the structure not touching to the content
 *
 * @param dynar poor victim
 *
 * kilkil a dynar BUT NOT its content. Ie, the array is freed, but the content is not touched (the @a free_f function
 * is not used)
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

/** @brief Frees the content and set the size to 0
 *
 * @param dynar who to squeeze
 */
void xbt_dynar_reset(xbt_dynar_t dynar)
{
  _sanity_check_dynar(dynar);

  XBT_CDEBUG(xbt_dyn, "Reset the dynar %p", (void *) dynar);
  if (dynar->free_f) {
    xbt_dynar_map(dynar, dynar->free_f);
  }
  dynar->used = 0;
}

/** @brief Merge dynar d2 into d1
 *
 * @param d1 dynar to keep
 * @param d2 dynar to merge into d1. This dynar is free at end.
 */
void xbt_dynar_merge(xbt_dynar_t* d1, xbt_dynar_t* d2)
{
  if((*d1)->elmsize != (*d2)->elmsize)
    xbt_die("Element size must are not equal");

  const unsigned long elmsize = (*d1)->elmsize;

  const void* ptr = _xbt_dynar_elm((*d2), 0);
  _xbt_dynar_resize(*d1, (*d1)->size + (*d2)->size);
  void *elm = _xbt_dynar_elm((*d1), (*d1)->used);

  memcpy(elm, ptr, ((*d2)->size)*elmsize);
  (*d1)->used += (*d2)->used;
  (*d2)->used = 0;
  xbt_dynar_free(d2);
}

/**
 * @brief Shrink the dynar by removing empty slots at the end of the internal array
 * @param dynar a dynar
 * @param empty_slots_wanted number of empty slots you want to keep at the end of the internal array for further
 * insertions
 *
 * Reduces the internal array size of the dynar to the number of elements plus @a empty_slots_wanted.
 * After removing elements from the dynar, you can call this function to make the dynar use less memory.
 * Set @a empty_slots_wanted to zero to reduce the dynar internal array as much as possible.
 * Note that if @a empty_slots_wanted is greater than the array size, the internal array is expanded instead of shrunk.
 */
void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots_wanted)
{
  _xbt_dynar_resize(dynar, dynar->used + empty_slots_wanted);
}

/** @brief Destructor
 *
 * @param dynar poor victim
 *
 * kilkil a dynar and its content
 */
void xbt_dynar_free(xbt_dynar_t* dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_reset(*dynar);
    xbt_dynar_free_container(dynar);
  }
}

/** @brief free a dynar passed as void* (handy to store dynar in dynars or dict) */
void xbt_dynar_free_voidp(void* d)
{
  xbt_dynar_t dynar = (xbt_dynar_t)d;
  xbt_dynar_free(&dynar);
}

/** @brief Count of dynar's elements
 *
 * @param dynar the dynar we want to measure
 */
unsigned long xbt_dynar_length(const_xbt_dynar_t dynar)
{
  return (dynar ? (unsigned long) dynar->used : (unsigned long) 0);
}

/**@brief check if a dynar is empty
 *
 *@param dynar the dynat we want to check
 */
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
 * @param dynar information dealer
 * @param idx index of the slot we want to retrieve
 * @return the @a idx-th element of @a dynar.
 *
 * @warning The returned value is the actual content of the dynar.
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

/** @brief Set the Nth element of a dynar (expanded if needed). Previous value at this position is NOT freed
 *
 * @param dynar information dealer
 * @param idx index of the slot we want to modify
 * @param src What will be feeded to the dynar
 *
 * If you want to free the previous content, use xbt_dynar_replace().
 */
void xbt_dynar_set(xbt_dynar_t dynar, int idx, const void* src)
{
  memcpy(xbt_dynar_set_at_ptr(dynar, idx), src, dynar->elmsize);
}

/** @brief Set the Nth element of a dynar (expanded if needed). Previous value is freed
 *
 * @param dynar
 * @param idx
 * @param object
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, AND DO free the previous value at this position. If
 * you don't want to free the previous content, use xbt_dynar_set().
 */
void xbt_dynar_replace(xbt_dynar_t dynar, unsigned long idx, const void* object)
{
  _sanity_check_dynar(dynar);

  if (idx < dynar->used && dynar->free_f) {
    void *const old_object = _xbt_dynar_elm(dynar, idx);

    dynar->free_f(old_object);
  }

  xbt_dynar_set(dynar, idx, object);
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

/** @brief Remove the Nth dynar's element, sliding the previous values to the left
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

/** @brief Remove a slice of the dynar, sliding the rest of the values to the left
 *
 * This function removes an n-sized slice that starts at element idx. It is equivalent to xbt_dynar_remove_at with a
 * nullptr object argument if n equals to 1.
 *
 * Each of the removed elements is freed using the free_f function passed at dynar creation.
 */
void xbt_dynar_remove_n_at(xbt_dynar_t dynar, unsigned int n, int idx)
{
  if (not n)
    return;

  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);
  _check_inbound_idx(dynar, idx + n - 1);

  if (dynar->free_f) {
    for (unsigned long cur = idx; cur < idx + n; cur++) {
      dynar->free_f(_xbt_dynar_elm(dynar, cur));
    }
  }

  unsigned long nb_shift = dynar->used - n - idx;

  if (nb_shift) {
    unsigned long offset = nb_shift * dynar->elmsize;
    memmove(_xbt_dynar_elm(dynar, idx), _xbt_dynar_elm(dynar, idx + n), offset);
  }

  dynar->used -= n;
}

/** @brief Returns the position of the element in the dynar
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function compares the
 * pointer value, not what's pointed. The only solution to search for a pointed value is then to write the foreach loop
 * yourself:
 * @code
 * signed int position = -1;
 * xbt_dynar_foreach(dynar, iter, elem) {
 *    if (not memcmp(elem, searched_element, sizeof(*elem))) {
 *        position = iter;
 *        break;
 *    }
 * }
 * @endcode
 *
 * Raises std::out_of_range if not found. If you have less than 2 millions elements, you probably want to use
 * #xbt_dynar_search_or_negative() instead, so that you don't have to try/catch on element not found.
 */
unsigned int xbt_dynar_search(const_xbt_dynar_t dynar, const void* elem)
{
  unsigned long it;

  for (it = 0; it < dynar->used; it++)
    if (not memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      return it;
    }

  throw std::out_of_range(simgrid::xbt::string_printf("Element %p not part of dynar %p", elem, dynar));
}

/** @brief Returns the position of the element in the dynar (or -1 if not found)
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function is probably not
 * what you want. Check the documentation of xbt_dynar_search() for more info.
 *
 * Note that usually, the dynar indices are unsigned integers. If you have more than 2 million elements in your dynar,
 * this very function will not work (but the other will).
 */
signed int xbt_dynar_search_or_negative(const_xbt_dynar_t dynar, const void* elem)
{
  unsigned long it;

  for (it = 0; it < dynar->used; it++)
    if (not memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      return it;
    }

  return -1;
}

/** @brief Returns a boolean indicating whether the element is part of the dynar
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function is probably not
 * what you want. Check the documentation of xbt_dynar_search() for more info.
 */
int xbt_dynar_member(const_xbt_dynar_t dynar, const void* elem)
{
  unsigned long it;

  for (it = 0; it < dynar->used; it++)
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
  char *const data = (char *) dynar->data;
  const unsigned long elmsize = dynar->elmsize;
  const unsigned long used = dynar->used;
  unsigned long i;

  _sanity_check_dynar(dynar);

  for (i = 0; i < used; i++) {
    char* elm = (char*) data + i * elmsize;
    op(elm);
  }
}

/** @brief Removes and free the entry pointed by the cursor
 *
 * This function can be used while traversing without problem.
 */
void xbt_dynar_cursor_rm(xbt_dynar_t dynar, unsigned int* cursor)
{
  xbt_dynar_remove_at(dynar, *cursor, nullptr);
  *cursor -= 1;
}

/** @brief Sorts a dynar according to the function <tt>compar_fn</tt>
 *
 * This function simply apply the classical qsort(3) function to the data stored in the dynar.
 * You should thus refer to the libc documentation, or to some online tutorial on how to write
 * a comparison function. Here is a quick example if you have integers in your dynar:
 *
 * @verbatim
 * int cmpfunc (const void * a, const void * b) {
 *   int intA = *(int*)a;
 *   int intB = *(int*)b;
 *   return intA - intB;
 * }
 * @endverbatim
 *
 * and now to sort a dynar of MSG hosts depending on their speed:
 * @verbatim
 * int cmpfunc(const MSG_host_t a, const MSG_host_t b) {
 *   MSG_host_t hostA = *(MSG_host_t*)a;
 *   MSG_host_t hostB = *(MSG_host_t*)b;
 *   return MSG_host_get_speed(hostA) - MSG_host_get_speed(hostB);
 * }
 * @endverbatim
 *
 * @param dynar the dynar to sort
 * @param compar_fn comparison function of type (int (compar_fn*) (const void*) (const void*)).
 */
void xbt_dynar_sort(const_xbt_dynar_t dynar, int_f_cpvoid_cpvoid_t compar_fn)
{
  if (dynar->data != nullptr)
    qsort(dynar->data, dynar->used, dynar->elmsize, compar_fn);
}

/** @brief Transform a dynar into a nullptr terminated array.
 *
 *  @param dynar the dynar to transform
 *  @return pointer to the first element of the array
 *
 *  Note: The dynar won't be usable afterwards.
 */
void* xbt_dynar_to_array(xbt_dynar_t dynar)
{
  void *res;
  xbt_dynar_shrink(dynar, 1);
  memset(xbt_dynar_push_ptr(dynar), 0, dynar->elmsize);
  res = dynar->data;
  xbt_free(dynar);
  return res;
}

/** @brief Compare two dynars
 *
 *  @param d1 first dynar to compare
 *  @param d2 second dynar to compare
 *  @param compar function to use to compare elements
 *  @return 0 if d1 and d2 are equal and 1 if not equal
 *
 *  d1 and d2 should be dynars of pointers. The compar function takes two  elements and returns 0 when they are
 *  considered equal, and a value different of zero when they are considered different. Finally, d2 is destroyed
 *  afterwards.
 */
int xbt_dynar_compare(const_xbt_dynar_t d1, xbt_dynar_t d2, int (*compar)(const void*, const void*))
{
  int i ;
  int size;
  if ((not d1) && (not d2))
    return 0;
  if ((not d1) || (not d2)) {
    XBT_DEBUG("nullptr dynar d1=%p d2=%p",d1,d2);
    xbt_dynar_free(&d2);
    return 1;
  }
  if((d1->elmsize)!=(d2->elmsize)) {
    XBT_DEBUG("Size of elmsize d1=%lu d2=%lu",d1->elmsize,d2->elmsize);
    xbt_dynar_free(&d2);
    return 1; // xbt_die
  }
  if(xbt_dynar_length(d1) != xbt_dynar_length(d2)) {
    XBT_DEBUG("Size of dynar d1=%lu d2=%lu",xbt_dynar_length(d1),xbt_dynar_length(d2));
    xbt_dynar_free(&d2);
    return 1;
  }

  size = xbt_dynar_length(d1);
  for(i=0;i<size;i++) {
    void *data1 = xbt_dynar_get_as(d1, i, void *);
    void *data2 = xbt_dynar_get_as(d2, i, void *);
    XBT_DEBUG("link[%d] d1=%p d2=%p",i,data1,data2);
    if(compar(data1,data2)){
      xbt_dynar_free(&d2);
      return 1;
    }
  }
  xbt_dynar_free(&d2);
  return 0;
}
