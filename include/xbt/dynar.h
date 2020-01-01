/* dynar - a generic dynamic array                                          */

/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_DYNAR_H
#define XBT_DYNAR_H

#include <string.h>             /* memcpy */

#include <xbt/base.h> /* SG_BEGIN_DECL */
#include <xbt/function_types.h>

SG_BEGIN_DECL

/** @addtogroup XBT_dynar
 * @brief DynArr are dynamically sized vector which may contain any type of variables.
 *
 * These are the SimGrid version of the dynamically size arrays, which all C programmer recode one day or another.
 *
 * For performance concerns, the content of DynArr must be homogeneous (in contrary to dictionaries -- see the
 * @ref XBT_dict section). You thus have to provide the function which will be used to free the content at
 * structure creation (of type void_f_pvoid_t).
 *
 * @deprecated If you are using C++, you might want to use `std::vector` instead.
 *
 * @section XBT_dynar_exscal Example with scalar
 * @dontinclude dynar.cpp
 *
 * @skip Vars_decl
 * @skip dyn
 * @until iptr
 * @skip Populate_ints
 * @skip dyn
 * @until end_of_traversal
 * @skip shifting
 * @skip val
 * @until xbt_dynar_free
 *
 * @section XBT_dynar_exptr Example with pointed data
 *
 * @skip test_dynar_string
 * @skip dynar_t
 * @until s2
 * @skip Populate_str
 * @skip dyn
 * @until }
 * @skip macro
 * @until dynar_free
 * @skip end_of_doxygen
 * @until }
 *
 * Note that if you use dynars to store pointed data, the xbt_dynar_search(), xbt_dynar_search_or_negative() and
 * xbt_dynar_member() won't be for you. Instead of comparing your pointed elements, they compare the pointer to them.
 * See the documentation of xbt_dynar_search() for more info.
 */
/** @defgroup XBT_dynar_cons Dynar constructor and destructor
 *  @ingroup XBT_dynar
 *
 *  @{
 */
/** @brief Dynar data type (opaque type) */
typedef struct xbt_dynar_s *xbt_dynar_t;
typedef const struct xbt_dynar_s* const_xbt_dynar_t;

XBT_PUBLIC xbt_dynar_t xbt_dynar_new(const unsigned long elm_size, void_f_pvoid_t free_f);
XBT_PUBLIC void xbt_dynar_init(xbt_dynar_t dynar, unsigned long elmsize, void_f_pvoid_t free_f);
XBT_PUBLIC void xbt_dynar_free(xbt_dynar_t* dynar);
XBT_PUBLIC void xbt_dynar_free_voidp(void* dynar);
XBT_PUBLIC void xbt_dynar_free_container(xbt_dynar_t* dynar);
XBT_PUBLIC void xbt_dynar_free_data(xbt_dynar_t dynar);
XBT_PUBLIC void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots);
XBT_PUBLIC void xbt_dynar_dump(const_xbt_dynar_t dynar);

/** @} */
/** @defgroup XBT_dynar_array Dynar as a regular array
 *  @ingroup XBT_dynar
 *
 *  @{
 */

XBT_PUBLIC void xbt_dynar_get_cpy(const_xbt_dynar_t dynar, unsigned long idx, void* dst);
XBT_PUBLIC void xbt_dynar_set(xbt_dynar_t dynar, int idx, const void* src);
XBT_PUBLIC void xbt_dynar_replace(xbt_dynar_t dynar, unsigned long idx, const void* object);

XBT_PUBLIC void xbt_dynar_insert_at(xbt_dynar_t dynar, int idx, const void* src);
XBT_PUBLIC void xbt_dynar_remove_at(xbt_dynar_t dynar, int idx, void* dst);
XBT_PUBLIC void xbt_dynar_remove_n_at(xbt_dynar_t dynar, unsigned int n, int idx);

XBT_PUBLIC unsigned int xbt_dynar_search(const_xbt_dynar_t dynar, const void* elem);
XBT_PUBLIC signed int xbt_dynar_search_or_negative(const_xbt_dynar_t dynar, const void* elem);
XBT_PUBLIC int xbt_dynar_member(const_xbt_dynar_t dynar, const void* elem);
XBT_PUBLIC void xbt_dynar_sort(const_xbt_dynar_t dynar, int_f_cpvoid_cpvoid_t compar_fn);
XBT_PUBLIC int xbt_dynar_compare(const_xbt_dynar_t d1, xbt_dynar_t d2, int (*compar)(const void*, const void*));
XBT_PUBLIC void* xbt_dynar_to_array(xbt_dynar_t dynar);

/** @} */
/** @defgroup XBT_dynar_misc Dynar miscellaneous functions
 *  @ingroup XBT_dynar
 *
 *  @{
 */

XBT_PUBLIC unsigned long xbt_dynar_length(const_xbt_dynar_t dynar);
XBT_PUBLIC int xbt_dynar_is_empty(const_xbt_dynar_t dynar);
XBT_PUBLIC void xbt_dynar_reset(xbt_dynar_t dynar);
XBT_PUBLIC void xbt_dynar_merge(xbt_dynar_t* d1, xbt_dynar_t* d2);

/** @} */
/** @defgroup XBT_dynar_perl Perl-like use of dynars
 *  @ingroup XBT_dynar
 *
 *  @{
 */

XBT_PUBLIC void xbt_dynar_push(xbt_dynar_t dynar, const void* src);
XBT_PUBLIC void xbt_dynar_pop(xbt_dynar_t dynar, void* dst);
XBT_PUBLIC void xbt_dynar_unshift(xbt_dynar_t dynar, const void* src);
XBT_PUBLIC void xbt_dynar_shift(xbt_dynar_t dynar, void* dst);
XBT_PUBLIC void xbt_dynar_map(const_xbt_dynar_t dynar, void_f_pvoid_t op);

/** @} */
/** @defgroup XBT_dynar_ctn Direct manipulation to the dynars content
 *  @ingroup XBT_dynar
 *
 *  Those functions do not retrieve the content, but only their address.
 *
 *  @{
 */

XBT_PUBLIC void* xbt_dynar_set_at_ptr(const xbt_dynar_t dynar, unsigned long idx);
XBT_PUBLIC void* xbt_dynar_get_ptr(const_xbt_dynar_t dynar, unsigned long idx);
XBT_PUBLIC void* xbt_dynar_insert_at_ptr(xbt_dynar_t dynar, int idx);
XBT_PUBLIC void* xbt_dynar_push_ptr(xbt_dynar_t dynar);
XBT_PUBLIC void* xbt_dynar_pop_ptr(xbt_dynar_t dynar);

/** @} */
/** @defgroup XBT_dynar_speed Speed optimized access to dynars of scalars
 *  @ingroup XBT_dynar
 *
 *  While the other functions use a memcpy to retrieve the content into the user provided area, those ones use a
 *  regular affectation. It only works for scalar values, but should be a little faster.
 *
 *  @{
 */

  /** @brief Quick retrieval of scalar content
   *  @hideinitializer */
#  define xbt_dynar_get_as(dynar,idx,type) \
          (*(type*)xbt_dynar_get_ptr((dynar),(idx)))
/** @brief Quick setting of scalar content
 *  @hideinitializer */
#define xbt_dynar_set_as(dynar, idx, type, val) (*(type*)xbt_dynar_set_at_ptr((dynar), (idx))) = (val)
/** @brief Quick retrieval of scalar content
 *  @hideinitializer */
#  define xbt_dynar_getlast_as(dynar,type) \
          (*(type*)xbt_dynar_get_ptr((dynar),xbt_dynar_length(dynar)-1))
/** @brief Quick retrieval of scalar content
 *  @hideinitializer */
#  define xbt_dynar_getfirst_as(dynar,type) \
          (*(type*)xbt_dynar_get_ptr((dynar),0))
/** @brief Quick insertion of scalar content
 *  @hideinitializer */
#define xbt_dynar_insert_at_as(dynar, idx, type, value) *(type*)xbt_dynar_insert_at_ptr((dynar), (idx)) = (value)
/** @brief Quick insertion of scalar content
 *  @hideinitializer */
#define xbt_dynar_push_as(dynar, type, value) *(type*)xbt_dynar_push_ptr(dynar) = (value)
/** @brief Quick removal of scalar content
 *  @hideinitializer */
#  define xbt_dynar_pop_as(dynar,type) \
           (*(type*)xbt_dynar_pop_ptr(dynar))

/** @} */
/** @defgroup XBT_dynar_cursor Cursors on dynar
 *  @ingroup XBT_dynar
 *
 * Cursors are used to iterate over the structure. Never add elements to the DynArr during the traversal. To remove
 * elements, use the xbt_dynar_cursor_rm() function.
 *
 * Do not call these function directly, but only within the xbt_dynar_foreach macro.
 *
 *  @{
 */

XBT_PUBLIC void xbt_dynar_cursor_rm(xbt_dynar_t dynar, unsigned int* cursor);

/*
 * @warning DO NOT USE THIS STRUCTURE DIRECTLY! Instead, use the public interface:
 *          This was made public to allow:
 *           - the inlining of the foreach elements
 *           - sending such beasts over the network
 *
 * @see xbt_dynar_length()
 */
typedef struct xbt_dynar_s {
  unsigned long size;
  unsigned long used;
  unsigned long elmsize;
  void *data;
  void_f_pvoid_t free_f;
} s_xbt_dynar_t;

static inline int _xbt_dynar_cursor_get(const_xbt_dynar_t dynar, unsigned int idx, void* dst)
{
  if (!dynar) /* iterating over a NULL dynar is a no-op */
    return 0;

  if (idx >= dynar->used)
    return 0;

  memcpy(dst, ((char *) dynar->data) + idx * dynar->elmsize, dynar->elmsize);

  return 1;
}

/** @brief Iterates over the whole dynar.
 *
 *  @param _dynar what to iterate over
 *  @param _cursor an integer used as cursor
 *  @param _data
 *  @hideinitializer
 *
 * Here is an example of usage:
 * @code
xbt_dynar_t dyn;
unsigned int cpt;
string *str;
xbt_dynar_foreach (dyn,cpt,str) {
  printf("Seen %s\n",str);
}
@endcode
 *
 * Note that underneath, that's a simple for loop with no real black  magic involved. It's perfectly safe to interrupt
 * a foreach with a break or a return statement.
 */
#define xbt_dynar_foreach(_dynar, _cursor, _data)                                                                      \
  for ((_cursor) = 0; _xbt_dynar_cursor_get((_dynar), (_cursor), &(_data)); (_cursor)++)

#ifndef __cplusplus
#define xbt_dynar_foreach_ptr(_dynar, _cursor, _ptr)                                                                   \
  for ((_cursor) = 0; ((_ptr) = (_cursor) < (_dynar)->used ? xbt_dynar_get_ptr((_dynar), (_cursor)) : NULL);           \
       (_cursor)++)
#else
#define xbt_dynar_foreach_ptr(_dynar, _cursor, _ptr)                                                                   \
  for ((_cursor) = 0;                                                                                                  \
       ((_ptr) = (_cursor) < (_dynar)->used ? (decltype(_ptr))xbt_dynar_get_ptr((_dynar), (_cursor)) : NULL);          \
       (_cursor)++)
#endif
/** @} */
SG_END_DECL

#endif /* XBT_DYNAR_H */
