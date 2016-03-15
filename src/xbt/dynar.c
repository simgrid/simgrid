/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include <sys/types.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dyn, xbt, "Dynamic arrays");

static inline void _sanity_check_dynar(xbt_dynar_t dynar)
{
  xbt_assert(dynar, "dynar is NULL");
}

static inline void _sanity_check_idx(int idx)
{
  xbt_assert(idx >= 0, "dynar idx(=%d) < 0", (int) (idx));
}

static inline void _check_inbound_idx(xbt_dynar_t dynar, int idx)
{
  if (idx < 0 || idx >= (int)dynar->used) {
    THROWF(bound_error, idx, "dynar is not that long. You asked %d, but it's only %lu long",
           (int) (idx), (unsigned long) dynar->used);
  }
}

static inline void _check_populated_dynar(xbt_dynar_t dynar)
{
  if (dynar->used == 0) {
    THROWF(bound_error, 0, "dynar %p is empty", dynar);
  }
}

static inline void _xbt_dynar_resize(xbt_dynar_t dynar, unsigned long new_size)
{
  if (new_size != dynar->size) {
    dynar->size = new_size;
    dynar->data = xbt_realloc(dynar->data, new_size * dynar->elmsize);
  }
}

static inline void _xbt_dynar_expand(xbt_dynar_t const dynar, const unsigned long nb)
{
  const unsigned long old_size = dynar->size;

  if (nb > old_size) {
    const unsigned long expand = 2 * (old_size + 1);
    _xbt_dynar_resize(dynar, (nb > expand ? nb : expand));
    XBT_DEBUG("expand %p from %lu to %lu elements", dynar, old_size, dynar->size);
  }
}

static inline void *_xbt_dynar_elm(const xbt_dynar_t dynar, const unsigned long idx)
{
  char *const data = (char *) dynar->data;
  const unsigned long elmsize = dynar->elmsize;

  return data + idx * elmsize;
}

static inline void _xbt_dynar_get_elm(void *const dst, const xbt_dynar_t dynar, const unsigned long idx)
{
  void *const elm = _xbt_dynar_elm(dynar, idx);

  memcpy(dst, elm, dynar->elmsize);
}

void xbt_dynar_dump(xbt_dynar_t dynar)
{
  XBT_INFO("Dynar dump: size=%lu; used=%lu; elmsize=%lu; data=%p; free_f=%p",
        dynar->size, dynar->used, dynar->elmsize, dynar->data, dynar->free_f);
}

/** @brief Constructor
 *
 * \param elmsize size of each element in the dynar
 * \param free_f function to call each time we want to get rid of an element (or NULL if nothing to do).
 *
 * Creates a new dynar. If a free_func is provided, the elements have to be pointer of pointer. That is to say that
 * dynars can contain either base types (int, char, double, etc) or pointer of pointers (struct **).
 */
xbt_dynar_t xbt_dynar_new(const unsigned long elmsize, void_f_pvoid_t const free_f)
{
  xbt_dynar_t dynar = xbt_new0(s_xbt_dynar_t, 1);

  dynar->size = 0;
  dynar->used = 0;
  dynar->elmsize = elmsize;
  dynar->data = NULL;
  dynar->free_f = free_f;

  return dynar;
}

/** @brief Destructor of the structure not touching to the content
 *
 * \param dynar poor victim
 *
 * kilkil a dynar BUT NOT its content. Ie, the array is freed, but the content is not touched (the \a free_f function
 * is not used)
 */
void xbt_dynar_free_container(xbt_dynar_t * dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_t d = *dynar;
    free(d->data);
    free(d);
    *dynar = NULL;
  }
}

/** @brief Frees the content and set the size to 0
 *
 * \param dynar who to squeeze
 */
inline void xbt_dynar_reset(xbt_dynar_t const dynar)
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
 * \param d1 dynar to keep
 * \param d2 dynar to merge into d1. This dynar is free at end.
 */
void xbt_dynar_merge(xbt_dynar_t *d1, xbt_dynar_t *d2)
{
  if((*d1)->elmsize != (*d2)->elmsize)
    xbt_die("Element size must are not equal");

  const unsigned long elmsize = (*d1)->elmsize;

  void *ptr = _xbt_dynar_elm((*d2), 0);
  _xbt_dynar_resize(*d1, (*d1)->size + (*d2)->size);
  void *elm = _xbt_dynar_elm((*d1), (*d1)->used);

  memcpy(elm, ptr, ((*d2)->size)*elmsize);
  (*d1)->used += (*d2)->used;
  (*d2)->used = 0;
  xbt_dynar_free(d2);
}

/**
 * \brief Shrink the dynar by removing empty slots at the end of the internal array
 * \param dynar a dynar
 * \param empty_slots_wanted number of empty slots you want to keep at the end of the internal array for further
 * insertions
 *
 * Reduces the internal array size of the dynar to the number of elements plus \a empty_slots_wanted.
 * After removing elements from the dynar, you can call this function to make the dynar use less memory.
 * Set \a empty_slots_wanted to zero to reduce the dynar internal array as much as possible.
 * Note that if \a empty_slots_wanted is greater than the array size, the internal array is expanded instead of shrunk.
 */
void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots_wanted)
{
  _xbt_dynar_resize(dynar, dynar->used + empty_slots_wanted);
}

/** @brief Destructor
 *
 * \param dynar poor victim
 *
 * kilkil a dynar and its content
 */
inline void xbt_dynar_free(xbt_dynar_t * dynar)
{
  if (dynar && *dynar) {
    xbt_dynar_reset(*dynar);
    xbt_dynar_free_container(dynar);
  }
}

/** \brief free a dynar passed as void* (handy to store dynar in dynars or dict) */
void xbt_dynar_free_voidp(void *d)
{
  xbt_dynar_t dynar = (xbt_dynar_t)d;
  xbt_dynar_free(&dynar);
}

/** @brief Count of dynar's elements
 *
 * \param dynar the dynar we want to mesure
 */
inline unsigned long xbt_dynar_length(const xbt_dynar_t dynar)
{
  return (dynar ? (unsigned long) dynar->used : (unsigned long) 0);
}

 /**@brief check if a dynar is empty
 *
 *\param dynar the dynat we want to check
 */

inline int xbt_dynar_is_empty(const xbt_dynar_t dynar)
{
  return (xbt_dynar_length(dynar) == 0);
}

/** @brief Retrieve a copy of the Nth element of a dynar.
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to retrieve
 * \param[out] dst where to put the result to.
 */
inline void xbt_dynar_get_cpy(const xbt_dynar_t dynar, const unsigned long idx, void *const dst)
{
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  _xbt_dynar_get_elm(dst, dynar, idx);
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
inline void *xbt_dynar_get_ptr(const xbt_dynar_t dynar, const unsigned long idx)
{
  void *res;
  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  res = _xbt_dynar_elm(dynar, idx);
  return res;
}

inline void *xbt_dynar_set_at_ptr(const xbt_dynar_t dynar, const unsigned long idx)
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
 * \param dynar information dealer
 * \param idx index of the slot we want to modify
 * \param src What will be feeded to the dynar
 *
 * If you want to free the previous content, use xbt_dynar_replace().
 */
inline void xbt_dynar_set(xbt_dynar_t dynar, const int idx, const void *const src)
{
  memcpy(xbt_dynar_set_at_ptr(dynar, idx), src, dynar->elmsize);
}

/** @brief Set the Nth element of a dynar (expanded if needed). Previous value is freed
 *
 * \param dynar
 * \param idx
 * \param object
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, AND DO free the previous value at this position. If
 * you don't want to free the previous content, use xbt_dynar_set().
 */
void xbt_dynar_replace(xbt_dynar_t dynar, const unsigned long idx, const void *const object)
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
void *xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar, const int idx)
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
inline void xbt_dynar_insert_at(xbt_dynar_t const dynar, const int idx, const void *const src)
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
void xbt_dynar_remove_at(xbt_dynar_t const dynar, const int idx, void *const object)
{
  unsigned long nb_shift;
  unsigned long offset;

  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);

  if (object) {
    _xbt_dynar_get_elm(object, dynar, idx);
  } else if (dynar->free_f) {
    dynar->free_f(_xbt_dynar_elm(dynar, idx));
  }

  nb_shift = dynar->used - 1 - idx;

  if (nb_shift) {
    offset = nb_shift * dynar->elmsize;
    memmove(_xbt_dynar_elm(dynar, idx), _xbt_dynar_elm(dynar, idx + 1), offset);
  }

  dynar->used--;
}

/** @brief Remove a slice of the dynar, sliding the rest of the values to the left
 *
 * This function removes an n-sized slice that starts at element idx. It is equivalent to xbt_dynar_remove_at with a
 * NULL object argument if n equals to 1.
 *
 * Each of the removed elements is freed using the free_f function passed at dynar creation.
 */
void xbt_dynar_remove_n_at(xbt_dynar_t const dynar, const unsigned int n, const int idx)
{
  unsigned long nb_shift;
  unsigned long offset;
  unsigned long cur;

  if (!n) return;

  _sanity_check_dynar(dynar);
  _check_inbound_idx(dynar, idx);
  _check_inbound_idx(dynar, idx + n - 1);

  if (dynar->free_f) {
    for (cur = idx; cur < idx + n; cur++) {
      dynar->free_f(_xbt_dynar_elm(dynar, cur));
    }
  }

  nb_shift = dynar->used - n - idx;

  if (nb_shift) {
    offset = nb_shift * dynar->elmsize;
    memmove(_xbt_dynar_elm(dynar, idx), _xbt_dynar_elm(dynar, idx + n), offset);
  }

  dynar->used -= n;
}

/** @brief Returns the position of the element in the dynar
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function compares the
 * pointer value, not what's pointed. The only solution to search for a pointed value is then to write the foreach loop
 * yourself:
 * \code
 * signed int position = -1;
 * xbt_dynar_foreach(dynar, iter, elem) {
 *    if (!memcmp(elem, searched_element, sizeof(*elem))) {
 *        position = iter;
 *        break;
 *    }
 * }
 * \endcode
 * 
 * Raises not_found_error if not found. If you have less than 2 millions elements, you probably want to use
 * #xbt_dynar_search_or_negative() instead, so that you don't have to TRY/CATCH on element not found.
 */
unsigned int xbt_dynar_search(xbt_dynar_t const dynar, void *const elem)
{
  unsigned long it;

  for (it = 0; it < dynar->used; it++)
    if (!memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      return it;
    }

  THROWF(not_found_error, 0, "Element %p not part of dynar %p", elem, dynar);
  return -1; // Won't happen, just to please eclipse
}

/** @brief Returns the position of the element in the dynar (or -1 if not found)
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function is probably not
 * what you want. Check the documentation of xbt_dynar_search() for more info.
 * 
 * Note that usually, the dynar indices are unsigned integers. If you have more than 2 million elements in your dynar,
 * this very function will not work (but the other will).
 */
signed int xbt_dynar_search_or_negative(xbt_dynar_t const dynar, void *const elem)
{
  unsigned long it;

  for (it = 0; it < dynar->used; it++)
    if (!memcmp(_xbt_dynar_elm(dynar, it), elem, dynar->elmsize)) {
      return it;
    }

  return -1;
}

/** @brief Returns a boolean indicating whether the element is part of the dynar 
 *
 * Beware that if your dynar contains pointed values (such as strings) instead of scalar, this function is probably not
 * what you want. Check the documentation of xbt_dynar_search() for more info.
 */
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
 * You can then use regular affectation to set its value instead of relying on the slow memcpy. This is what
 * xbt_dynar_push_as() does.
 */
inline void *xbt_dynar_push_ptr(xbt_dynar_t const dynar)
{
  return xbt_dynar_insert_at_ptr(dynar, dynar->used);
}

/** @brief Add an element at the end of the dynar */
inline void xbt_dynar_push(xbt_dynar_t const dynar, const void *const src)
{
  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(xbt_dynar_insert_at_ptr(dynar, dynar->used), src, dynar->elmsize);
}

/** @brief Mark the last dynar's element as unused and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying on the slow memcpy. This is what
 * xbt_dynar_pop_as() does.
 */
inline void *xbt_dynar_pop_ptr(xbt_dynar_t const dynar)
{
  _check_populated_dynar(dynar);
  XBT_CDEBUG(xbt_dyn, "Pop %p", (void *) dynar);
  dynar->used--;
  return _xbt_dynar_elm(dynar, dynar->used);
}

/** @brief Get and remove the last element of the dynar */
inline void xbt_dynar_pop(xbt_dynar_t const dynar, void *const dst)
{
  /* sanity checks done by remove_at */
  XBT_CDEBUG(xbt_dyn, "Pop %p", (void *) dynar);
  xbt_dynar_remove_at(dynar, dynar->used - 1, dst);
}

/** @brief Add an element at the begining of the dynar.
 *
 * This is less efficient than xbt_dynar_push()
 */
inline void xbt_dynar_unshift(xbt_dynar_t const dynar, const void *const src)
{
  /* sanity checks done by insert_at */
  xbt_dynar_insert_at(dynar, 0, src);
}

/** @brief Get and remove the first element of the dynar.
 *
 * This is less efficient than xbt_dynar_pop()
 */
inline void xbt_dynar_shift(xbt_dynar_t const dynar, void *const dst)
{
  /* sanity checks done by remove_at */
  xbt_dynar_remove_at(dynar, 0, dst);
}

/** @brief Apply a function to each member of a dynar
 *
 * The mapped function may change the value of the element itself, but should not mess with the structure of the dynar.
 */
inline void xbt_dynar_map(const xbt_dynar_t dynar, void_f_pvoid_t const op)
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
inline void xbt_dynar_cursor_rm(xbt_dynar_t dynar, unsigned int *const cursor)
{
  xbt_dynar_remove_at(dynar, (*cursor)--, NULL);
}

/** @brief Sorts a dynar according to the function <tt>compar_fn</tt>
 *
 * \param dynar the dynar to sort
 * \param compar_fn comparison function of type (int (compar_fn*) (void*) (void*)).
 *
 * Remark: if the elements stored in the dynar are structures, the compar_fn function has to retrieve the field to sort
 * first.
 */
inline void xbt_dynar_sort(xbt_dynar_t dynar, int_f_cpvoid_cpvoid_t compar_fn)
{
  qsort(dynar->data, dynar->used, dynar->elmsize, compar_fn);
}

static int strcmp_voidp(const void *pa, const void *pb) {
  return strcmp(*(const char **)pa, *(const char **)pb);
}

/** @brief Sorts a dynar of strings (ie, char* data) */
xbt_dynar_t xbt_dynar_sort_strings(xbt_dynar_t dynar)
{
  xbt_dynar_sort(dynar, strcmp_voidp);
  return dynar; // to enable functional uses
}

/** @brief Sorts a dynar according to their color assuming elements can have only three colors.
 * Since there are only three colors, it is linear and much faster than a classical sort.
 * See for example http://en.wikipedia.org/wiki/Dutch_national_flag_problem
 *
 * \param dynar the dynar to sort
 * \param color the color function of type (int (compar_fn*) (void*) (void*)). The return value of color is assumed to
 *        be 0, 1, or 2.
 *
 * At the end of the call, elements with color 0 are at the beginning of the dynar, elements with color 2 are at the
 * end and elements with color 1 are in the middle.
 *
 * Remark: if the elements stored in the dynar are structures, the color function has to retrieve the field to sort
 * first.
 */
XBT_PUBLIC(void) xbt_dynar_three_way_partition(xbt_dynar_t const dynar, int_f_pvoid_t color)
{
  unsigned long int i;
  unsigned long int p = -1;
  unsigned long int q = dynar->used;
  const unsigned long elmsize = dynar->elmsize;
  void *tmp = xbt_malloc(elmsize);
  void *elm;

  for (i = 0; i < q;) {
    void *elmi = _xbt_dynar_elm(dynar, i);
    int colori = color(elmi);

    if (colori == 1) {
      ++i;
    } else {
      if (colori == 0) {
        elm = _xbt_dynar_elm(dynar, ++p);
        ++i;
      } else {                  /* colori == 2 */
        elm = _xbt_dynar_elm(dynar, --q);
      }
      if (elm != elmi) {
        memcpy(tmp,  elm,  elmsize);
        memcpy(elm,  elmi, elmsize);
        memcpy(elmi, tmp,  elmsize);
      }
    }
  }
  xbt_free(tmp);
}

/** @brief Transform a dynar into a NULL terminated array. 
 *
 *  \param dynar the dynar to transform
 *  \return pointer to the first element of the array
 *
 *  Note: The dynar won't be usable afterwards.
 */
inline void *xbt_dynar_to_array(xbt_dynar_t dynar)
{
  void *res;
  xbt_dynar_shrink(dynar, 1);
  memset(xbt_dynar_push_ptr(dynar), 0, dynar->elmsize);
  res = dynar->data;
  free(dynar);
  return res;
}

/** @brief Compare two dynars
 *
 *  \param d1 first dynar to compare
 *  \param d2 second dynar to compare
 *  \param compar function to use to compare elements
 *  \return 0 if d1 and d2 are equal and 1 if not equal
 *
 *  d1 and d2 should be dynars of pointers. The compar function takes two  elements and returns 0 when they are
 *  considered equal, and a value different of zero when they are considered different. Finally, d2 is destroyed
 *  afterwards.
 */
int xbt_dynar_compare(xbt_dynar_t d1, xbt_dynar_t d2, int(*compar)(const void *, const void *))
{
  int i ;
  int size;
  if((!d1) && (!d2)) return 0;
  if((!d1) || (!d2))
  {
    XBT_DEBUG("NULL dynar d1=%p d2=%p",d1,d2);
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

#ifdef SIMGRID_TEST

#define NB_ELEM 5000

XBT_TEST_SUITE("dynar", "Dynar data container");
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_dyn);

XBT_TEST_UNIT("int", test_dynar_int, "Dynars of integers")
{
  /* Vars_decl [doxygen cruft] */
  xbt_dynar_t d;
  int i, cpt;
  unsigned int cursor;
  int *iptr;

  xbt_test_add("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, i) {
    xbt_die( "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push %d int, set them again 3 times, traverse them, shift them", NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);     /* This is faster (and possible only with scalars) */
    /* xbt_dynar_push(d,&cpt);       This would also work */
    xbt_test_log("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 2. Traverse manually the dynar */
  for (cursor = 0; cursor < NB_ELEM; cursor++) {
    iptr = xbt_dynar_get_ptr(d, cursor);
    xbt_test_assert(cursor == *iptr, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
  }

  /* 3. Traverse the dynar using the neat macro to that extend */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert(cursor == cpt, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
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
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one (%d!=%d)", i, cpt);
    cpt++;
  }
  xbt_test_assert(cpt == NB_ELEM, "Cannot retrieve my %d values. Last got one is %d", NB_ELEM, cpt);

  /* shifting [doxygen cruft] */
  /* 4. Shift all the values */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one (%d!=%d)", i, cpt);
    xbt_test_log("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  int* pi;
  xbt_dynar_foreach_ptr(d, cursor, pi) {
    *pi = 0;
  }
  xbt_dynar_foreach(d, cursor, i) {
    xbt_test_assert(i == 0, "The value is not the same as the expected one.");
  }
  xbt_dynar_foreach_ptr(d, cursor, pi) {
    *pi = 1;
  }
  xbt_dynar_foreach(d, cursor, i) {
    xbt_test_assert(i == 1, "The value is not the same as the expected one.");
  }

  /* 5. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Unshift/pop %d int", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_unshift(d, &cpt);
    XBT_DEBUG("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    i = xbt_dynar_pop_as(d, int);
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one (%d!=%d)", i, cpt);
    xbt_test_log("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add ("==== Push %d int, insert 1000 int in the middle, shift everything", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);
    XBT_DEBUG("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM/5; cpt++) {
    xbt_dynar_insert_at_as(d, NB_ELEM/2, int, cpt);
    XBT_DEBUG("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  for (cpt = 0; cpt < NB_ELEM/2; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
                     i, cpt);
    XBT_DEBUG("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
                     i, cpt);
  }
  for (cpt = 2500; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert(i == cpt, "The retrieved value is not the same than the injected one at the end (%d!=%d)", i, cpt);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push %d int, remove 2000-4000. free the rest", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_push_as(d, int, cpt);

  for (cpt = 2000; cpt < 4000; cpt++) {
    xbt_dynar_remove_at(d, 2000, &i);
    xbt_test_assert(i == cpt, "Remove a bad value. Got %d, expected %d", i, cpt);
    XBT_DEBUG("remove %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}

/*******************************************************************************/
XBT_TEST_UNIT("insert",test_dynar_insert,"Using the xbt_dynar_insert and xbt_dynar_remove functions")
{
  xbt_dynar_t d = xbt_dynar_new(sizeof(unsigned int), NULL);
  unsigned int cursor;
  int cpt;

  xbt_test_add("==== Insert %d int, traverse them, remove them",NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_insert_at(d, cpt, &cpt);
    xbt_test_log("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert(cursor == cpt, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
  }
  /* end_of_traversal */

  /* Re-fill with the same values using set_as (and re-verify) */
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_set_as(d, cpt, int, cpt);
  xbt_dynar_foreach(d, cursor, cpt)
    xbt_test_assert(cursor == cpt, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);

  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    int val;
    xbt_dynar_remove_at(d,0,&val);
    xbt_test_assert(cpt == val, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
  }
  xbt_test_assert(xbt_dynar_is_empty(d), "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);

  /* ********************* */
  xbt_test_add("==== Insert %d int in reverse order, traverse them, remove them",NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = NB_ELEM-1; cpt >=0; cpt--) {
    xbt_dynar_replace(d, cpt, &cpt);
    xbt_test_log("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert(cursor == cpt, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
  }
  /* end_of_traversal */

  for (cpt =NB_ELEM-1; cpt >=0; cpt--) {
    int val;
    xbt_dynar_remove_at(d,xbt_dynar_length(d)-1,&val);
    xbt_test_assert(cpt == val, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
  }
  xbt_test_assert(xbt_dynar_is_empty(d), "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);
}

/*******************************************************************************/
XBT_TEST_UNIT("double", test_dynar_double, "Dynars of doubles")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int cursor;
  double d1, d2;

  xbt_test_add("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert(FALSE, "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push/shift 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  xbt_dynar_foreach(d, cursor, d2) {
    d1 = (double) cursor;
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one (%f!=%f)", d1, d2);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one (%f!=%f)", d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Unshift/pop 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_unshift(d, &d1);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_pop(d, &d2);
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one (%f!=%f)", d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
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
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one at the begining (%f!=%f)",
                     d1, d2);
    XBT_DEBUG("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one in the middle (%f!=%f)",
                     d1, d2);
  }
  for (cpt = 2500; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert(d1 == d2, "The retrieved value is not the same than the injected one at the end (%f!=%f)", d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push 5000 double, remove 2000-4000. free the rest");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  for (cpt = 2000; cpt < 4000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_remove_at(d, 2000, &d2);
    xbt_test_assert(d1 == d2, "Remove a bad value. Got %f, expected %f", d2, d1);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}

/* doxygen_string_cruft */

/*******************************************************************************/
XBT_TEST_UNIT("string", test_dynar_string, "Dynars of strings")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int iter;
  char buf[1024];
  char *s1, *s2;

  xbt_test_add("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  xbt_dynar_foreach(d, iter, s1) {
    xbt_test_assert(FALSE, "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push %d strings, set them again 3 times, shift them", NB_ELEM);
  /* Populate_str [doxygen cruft] */
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert(!strcmp(buf, s2), "The retrieved value is not the same than the injected one (%s!=%s)", buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Unshift, traverse and pop %d strings", NB_ELEM);
  d = xbt_dynar_new(sizeof(char **), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_unshift(d, &s1);
  }
  /* 2. Traverse the dynar with the macro */
  xbt_dynar_foreach(d, iter, s1) {
    sprintf(buf, "%u", NB_ELEM - iter - 1);
    xbt_test_assert(!strcmp(buf, s1), "The retrieved value is not the same than the injected one (%s!=%s)", buf, s1);
  }
  /* 3. Traverse the dynar with the macro */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_pop(d, &s2);
    xbt_test_assert(!strcmp(buf, s2), "The retrieved value is not the same than the injected one (%s!=%s)", buf, s2);
    free(s2);
  }
  /* 4. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push %d strings, insert %d strings in the middle, shift everything", NB_ELEM, NB_ELEM / 5);
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM / 5; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_insert_at(d, NB_ELEM / 2, &s1);
  }

  for (cpt = 0; cpt < NB_ELEM / 2; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one at the begining (%s!=%s)", buf, s2);
    free(s2);
  }
  for (cpt = (NB_ELEM / 5) - 1; cpt >= 0; cpt--) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one in the middle (%s!=%s)", buf, s2);
    free(s2);
  }
  for (cpt = NB_ELEM / 2; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert(!strcmp(buf, s2), "The retrieved value is not the same than the injected one at the end (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add("==== Push %d strings, remove %d-%d. free the rest", NB_ELEM, 2 * (NB_ELEM / 5), 4 * (NB_ELEM / 5));
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = xbt_strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 2 * (NB_ELEM / 5); cpt < 4 * (NB_ELEM / 5); cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_remove_at(d, 2 * (NB_ELEM / 5), &s2);
    xbt_test_assert(!strcmp(buf, s2), "Remove a bad value. Got %s, expected %s", s2, buf);
    free(s2);
  }
  xbt_dynar_free(&d);           /* end_of_doxygen */
}
#endif                          /* SIMGRID_TEST */
