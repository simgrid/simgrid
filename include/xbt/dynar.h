/* dynar - a generic dynamic array                                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DYNAR_H
#define _XBT_DYNAR_H

#include <string.h> /* memcpy */

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_dynar
  * @brief DynArr are dynamically sized vector which may contain any type of variables.
  *
  * These are the SimGrid version of the dynamically size arrays, which all C programmer recode one day or another.
  *  
  * For performance concerns, the content of DynArr must be homogeneous (in
  * contrary to dictionnaries -- see the \ref XBT_dict section). You thus
  * have to provide the function which will be used to free the content at
  * structure creation (of type void_f_ppvoid_t or void_f_pvoid_t).
  *
  * \section XBT_dynar_exscal Example with scalar
  * \dontinclude dynar.c
  *
  * \skip Vars_decl
  * \skip dyn
  * \until iptr
  * \skip Populate_ints
  * \skip dyn
  * \until end_of_traversal
  * \skip shifting
  * \skip val
  * \until xbt_dynar_free
  *
  * \section XBT_dynar_exptr Example with pointed data
  * 
  * \skip test_dynar_string
  * \skip dynar_t
  * \until s2
  * \skip Populate_str
  * \skip dyn
  * \until }
  * \skip macro
  * \until dynar_free
  * \skip end_of_doxygen
  * \until }
  *
  */
/** @defgroup XBT_dynar_cons Dynar constructor and destructor
 *  @ingroup XBT_dynar
 *
 *  @{
 */
   /** \brief Dynar data type (opaque type) */
     typedef struct xbt_dynar_s *xbt_dynar_t;


XBT_PUBLIC(xbt_dynar_t) xbt_dynar_new(const unsigned long elm_size,
                                      void_f_pvoid_t const free_f);
XBT_PUBLIC(xbt_dynar_t) xbt_dynar_new_sync(const unsigned long elm_size,
                                           void_f_pvoid_t const free_f);
XBT_INLINE XBT_PUBLIC(void) xbt_dynar_free(xbt_dynar_t * dynar);
XBT_PUBLIC(void) xbt_dynar_free_voidp(void *dynar);
XBT_PUBLIC(void) xbt_dynar_free_container(xbt_dynar_t * dynar);

XBT_INLINE XBT_PUBLIC(unsigned long) xbt_dynar_length(const xbt_dynar_t dynar);
XBT_PUBLIC(void) xbt_dynar_reset(xbt_dynar_t const dynar);
XBT_PUBLIC(void) xbt_dynar_shrink(xbt_dynar_t dynar, int empty_slots);

XBT_PUBLIC(void) xbt_dynar_dump(xbt_dynar_t dynar);

/** @} */
/** @defgroup XBT_dynar_array Dynar as a regular array
 *  @ingroup XBT_dynar
 *
 *  @{
 */

XBT_PUBLIC(void) xbt_dynar_get_cpy(const xbt_dynar_t dynar,
                                   const unsigned long idx, void *const dst);

XBT_PUBLIC(void) xbt_dynar_set(xbt_dynar_t dynar, const int idx,
                               const void *src);
XBT_PUBLIC(void) xbt_dynar_replace(xbt_dynar_t dynar, const unsigned long idx,
                                   const void *object);

XBT_PUBLIC(void) xbt_dynar_insert_at(xbt_dynar_t const dynar, const int idx,
                                     const void *src);
XBT_PUBLIC(void) xbt_dynar_remove_at(xbt_dynar_t const dynar, const int idx,
                                     void *const dst);

XBT_PUBLIC(int) xbt_dynar_search(xbt_dynar_t const dynar, void *elem);
XBT_PUBLIC(int) xbt_dynar_member(xbt_dynar_t const dynar, void *elem);
XBT_PUBLIC(void) xbt_dynar_sort(xbt_dynar_t const dynar, int_f_cpvoid_cpvoid_t compar_fn);

/** @} */
/** @defgroup XBT_dynar_perl Perl-like use of dynars
 *  @ingroup XBT_dynar
 *
 *  @{
 */

XBT_INLINE XBT_PUBLIC(void) xbt_dynar_push(xbt_dynar_t const dynar, const void *src);
XBT_INLINE XBT_PUBLIC(void) xbt_dynar_pop(xbt_dynar_t const dynar, void *const dst);
XBT_PUBLIC(void) xbt_dynar_unshift(xbt_dynar_t const dynar, const void *src);
XBT_PUBLIC(void) xbt_dynar_shift(xbt_dynar_t const dynar, void *const dst);
XBT_PUBLIC(void) xbt_dynar_map(const xbt_dynar_t dynar,
                               void_f_pvoid_t const op);

/** @} */
/** @defgroup XBT_dynar_ctn Direct manipulation to the dynars content
 *  @ingroup XBT_dynar
 *
 *  Those functions do not retrieve the content, but only their address.
 *
 *  @{
 */

XBT_INLINE XBT_PUBLIC(void *) xbt_dynar_get_ptr(const xbt_dynar_t dynar,
                                     const unsigned long idx);
XBT_PUBLIC(void *) xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar,
                                           const int idx);
XBT_PUBLIC(void *) xbt_dynar_push_ptr(xbt_dynar_t const dynar);
XBT_PUBLIC(void *) xbt_dynar_pop_ptr(xbt_dynar_t const dynar);

/** @} */
/** @defgroup XBT_dynar_speed Speed optimized access to dynars of scalars
 *  @ingroup XBT_dynar
 *
 *  While the other functions use a memcpy to retrieve the content into the
 *  user provided area, those ones use a regular affectation. It only works
 *  for scalar values, but should be a little faster.
 *
 *  @{
 */

  /** @brief Quick retrieval of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_get_as(dynar,idx,type) \
          (*(type*)xbt_dynar_get_ptr((dynar),(idx)))
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
#  define xbt_dynar_insert_at_as(dynar,idx,type,value) \
          *(type*)xbt_dynar_insert_at_ptr(dynar,idx)=value
  /** @brief Quick insertion of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_push_as(dynar,type,value) \
          *(type*)xbt_dynar_push_ptr(dynar)=value
  /** @brief Quick removal of scalar content
   *  @hideinitializer */
#  define xbt_dynar_pop_as(dynar,type) \
           (*(type*)xbt_dynar_pop_ptr(dynar))

/** @} */
/** @defgroup XBT_dynar_cursor Cursors on dynar
 *  @ingroup XBT_dynar
 *
 * Cursors are used to iterate over the structure. Never add elements to the 
 * DynArr during the traversal. To remove elements, use the
 * xbt_dynar_cursor_rm() function.
 *
 * Do not call these functions directly, but only the xbt_dynar_foreach macro.
 * 
 * For synchronized dynars, the dynar will be locked during the whole
 * loop and it will get unlocked automatically if you traverse all
 * elements. If you want to break the loop before the end, make sure
 * to call xbt_dynar_cursor_unlock() before the <tt>break;</tt>
 *
 *  @{
 */

XBT_INLINE XBT_PUBLIC(void) xbt_dynar_cursor_rm(xbt_dynar_t dynar,
                                     unsigned int *const cursor);
XBT_PUBLIC(void) xbt_dynar_cursor_unlock(xbt_dynar_t dynar);

/* do not use this structure internals directly, but use the public interface
 * This was made public to allow:
 *  - the inlining of the foreach elements
 *  - sending such beasts over the network
 */

#include "xbt/synchro_core.h"
typedef struct xbt_dynar_s {
  unsigned long size;
  unsigned long used;
  unsigned long elmsize;
  void *data;
  void_f_pvoid_t free_f;
  xbt_mutex_t mutex;
} s_xbt_dynar_t;

static XBT_INLINE void
_xbt_dynar_cursor_first(const xbt_dynar_t dynar, unsigned int *const cursor)
{
  /* don't test for dynar!=NULL. The segfault would tell us */
  if (dynar->mutex)  /* ie _dynar_lock(dynar) but not public */
    xbt_mutex_acquire(dynar->mutex);

  //DEBUG1("Set cursor on %p to the first position", (void *) dynar);
  *cursor = 0;
}

static XBT_INLINE int
_xbt_dynar_cursor_get(const xbt_dynar_t dynar,
                      unsigned int idx, void *const dst)
{

  if (idx >= dynar->used) {
    //DEBUG1("Cursor on %p already on last elem", (void *) dynar);
    if (dynar->mutex) /* unlock */
      xbt_mutex_release(dynar->mutex);
    return FALSE;
  }
  //  DEBUG2("Cash out cursor on %p at %u", (void *) dynar, *idx);

  memcpy(dst, ((char*)dynar->data) + idx * dynar->elmsize, dynar->elmsize);

  return TRUE;
}



/** @brief Iterates over the whole dynar. 
 * 
 *  @param _dynar what to iterate over
 *  @param _cursor an integer used as cursor
 *  @param _data
 *  @hideinitializer
 *
 * \note An example of usage:
 * \code
xbt_dynar_t dyn;
unsigned int cpt;
string *str;
xbt_dynar_foreach (dyn,cpt,str) {
  printf("Seen %s\n",str);
}
\endcode
 */
#define xbt_dynar_foreach(_dynar,_cursor,_data) \
       for (_xbt_dynar_cursor_first(_dynar,&(_cursor))      ; \
	    _xbt_dynar_cursor_get(_dynar,_cursor,&_data) ; \
            (_cursor)++         )

/** @} */

SG_END_DECL()
#endif /* _XBT_DYNAR_H */
