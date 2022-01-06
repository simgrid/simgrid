/* dynar - a generic dynamic array                                          */

/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_DYNAR_H
#define XBT_DYNAR_H

#include <string.h>             /* memcpy */

#include <xbt/base.h> /* SG_BEGIN_DECL */
#include <xbt/function_types.h>

SG_BEGIN_DECL

/** @brief Dynar data type (opaque type)
 *
 * These are the SimGrid version of the DYNamically sized ARays, which all C programmer recode one day or another.
 * For performance concerns, the content of dynars must be homogeneous (in contrary to @ref xbt_dict_t).
 * You thus have to provide the function which will be used to free the content at
 * structure creation (of type void_f_pvoid_t).
 */
typedef struct xbt_dynar_s *xbt_dynar_t;
/** constant dynar */
typedef const struct xbt_dynar_s* const_xbt_dynar_t;
/* @warning DO NOT USE THIS STRUCTURE DIRECTLY! Use the public interface, even if it's public for sake of inlining */
typedef struct xbt_dynar_s {
  unsigned long size;
  unsigned long used;
  unsigned long elmsize;
  void* data;
  void_f_pvoid_t free_f;
} s_xbt_dynar_t;

XBT_PUBLIC xbt_dynar_t xbt_dynar_new(const unsigned long elm_size, void_f_pvoid_t free_f);
XBT_PUBLIC void xbt_dynar_free(xbt_dynar_t* dynar);
XBT_PUBLIC void xbt_dynar_free_container(xbt_dynar_t* dynar);
XBT_PUBLIC void xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots);

/* Dynar as a regular array */
XBT_PUBLIC void xbt_dynar_get_cpy(const_xbt_dynar_t dynar, unsigned long idx, void* dst);

XBT_PUBLIC void xbt_dynar_insert_at(xbt_dynar_t dynar, int idx, const void* src);
XBT_PUBLIC void xbt_dynar_remove_at(xbt_dynar_t dynar, int idx, void* dst);

XBT_PUBLIC int xbt_dynar_member(const_xbt_dynar_t dynar, const void* elem);
XBT_PUBLIC void xbt_dynar_sort(const_xbt_dynar_t dynar, int_f_cpvoid_cpvoid_t compar_fn);
XBT_ATTRIB_DEPRECATED_v331("This function will be removed") XBT_PUBLIC void* xbt_dynar_to_array(xbt_dynar_t dynar);

/* Dynar size */
XBT_PUBLIC void xbt_dynar_reset(xbt_dynar_t dynar);
XBT_PUBLIC unsigned long xbt_dynar_length(const_xbt_dynar_t dynar);
XBT_PUBLIC int xbt_dynar_is_empty(const_xbt_dynar_t dynar);

/* Perl-like interface */
XBT_PUBLIC void xbt_dynar_push(xbt_dynar_t dynar, const void* src);
XBT_PUBLIC void xbt_dynar_pop(xbt_dynar_t dynar, void* dst);
XBT_PUBLIC void xbt_dynar_unshift(xbt_dynar_t dynar, const void* src);
XBT_PUBLIC void xbt_dynar_shift(xbt_dynar_t dynar, void* dst);
XBT_PUBLIC void xbt_dynar_map(const_xbt_dynar_t dynar, void_f_pvoid_t op);

/* Direct content manipulation*/
XBT_PUBLIC void* xbt_dynar_set_at_ptr(const xbt_dynar_t dynar, unsigned long idx);
XBT_PUBLIC void* xbt_dynar_get_ptr(const_xbt_dynar_t dynar, unsigned long idx);
XBT_PUBLIC void* xbt_dynar_insert_at_ptr(xbt_dynar_t dynar, int idx);
XBT_PUBLIC void* xbt_dynar_push_ptr(xbt_dynar_t dynar);
XBT_PUBLIC void* xbt_dynar_pop_ptr(xbt_dynar_t dynar);

/* Dynars of scalar */
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
#define xbt_dynar_push_as(dynar, type, value) *(type*)xbt_dynar_push_ptr(dynar) = (value)
/** @brief Quick removal of scalar content
 *  @hideinitializer */
#  define xbt_dynar_pop_as(dynar,type) \
           (*(type*)xbt_dynar_pop_ptr(dynar))

/* Iterating over dynars */
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
 *  @param _cursor an unsigned integer used as cursor
 *  @hideinitializer
 *
 * Here is an example of usage:
 * @code
xbt_dynar_t dyn;
unsigned int cpt;
char* str;
xbt_dynar_foreach (dyn,cpt,str) {
  printf("Seen %s\n",str);
}
@endcode
 *
 * Note that underneath, that's a simple for loop with no real black  magic involved. It's perfectly safe to interrupt
 * a foreach with a break or a return statement, but inserting or removing elements during the traversal is probably a
 * bad idea (even if you can fix side effects by manually changing the counter).
 */
#define xbt_dynar_foreach(_dynar, _cursor, _data)                                                                      \
  for ((_cursor) = 0; _xbt_dynar_cursor_get((_dynar), (_cursor), &(_data)); (_cursor)++)
SG_END_DECL

#endif /* XBT_DYNAR_H */
