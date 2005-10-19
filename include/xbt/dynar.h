/* $Id$ */

/* dynar - a generic dynamic array                                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DYNAR_H
#define _XBT_DYNAR_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

/** \addtogroup XBT_dynar
  *  
  * For performance concerns, the content of DynArr must be homogeneous (in
  * contrary to dictionnaries -- see the \ref XBT_dict section). You thus
  * have to provide the function which will be used to free the content at
  * structure creation (of type void_f_ppvoid_t or void_f_pvoid_t).
  *
  * \section XBT_dynar_exscal Example with scalar
  * \dontinclude dynar_int.c
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
  * \dontinclude dynar_string.c
  * 
  * \skip doxygen_first_cruft
  * \skip f
  * \until xbt_init
  * \skip Populate_str
  * \skip dyn
  * \until }
  * \skip macro
  * \until dynar_free
  * \skip xbt_exit
  * \until }
  * @{
  */

/** @name 1. Constructor/destructor
 *  @{
 */
   /** \brief Dynar data type (opaque type) */
   typedef struct xbt_dynar_s *xbt_dynar_t;


  xbt_dynar_t   xbt_dynar_new(unsigned long elm_size, 
			     void_f_pvoid_t *free_func);
  void          xbt_dynar_free(xbt_dynar_t *dynar);
  void          xbt_dynar_free_container(xbt_dynar_t *dynar);

  unsigned long xbt_dynar_length(const xbt_dynar_t dynar);
  void          xbt_dynar_reset(xbt_dynar_t dynar);

  void          xbt_dynar_dump(xbt_dynar_t dynar);

/** @} */
/** @name 2. regular array functions
 *  @{
 */

  void xbt_dynar_get_cpy(const xbt_dynar_t dynar, int idx, void * const dst);
  
  void xbt_dynar_set(xbt_dynar_t dynar, int idx, const void *src);
  void xbt_dynar_replace(xbt_dynar_t dynar, int idx, const void *object);

  void xbt_dynar_insert_at(xbt_dynar_t dynar, int  idx, const void *src);
  void xbt_dynar_remove_at(xbt_dynar_t dynar, int  idx, void * const dst);

/** @} */
/** @name 2. Perl-like functions
 *  @{
 */

  void xbt_dynar_push    (xbt_dynar_t dynar, const void *src);
  void xbt_dynar_pop     (xbt_dynar_t dynar, void *const dst);
  void xbt_dynar_unshift (xbt_dynar_t dynar, const void *src);
  void xbt_dynar_shift   (xbt_dynar_t dynar, void *const dst);
  void xbt_dynar_map     (const xbt_dynar_t dynar, void_f_pvoid_t *op);

/** @} */
/** @name 3. Manipulating pointers to the content
 *
 *  Those functions do not retrive the content, but only their address.
 *
 *  @{
 */

  void *xbt_dynar_get_ptr(const xbt_dynar_t dynar, const int idx);
  void *xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar, const int idx);
  void *xbt_dynar_push_ptr(xbt_dynar_t dynar);
  void *xbt_dynar_pop_ptr(xbt_dynar_t dynar);

/** @} */
/** @name 4. Speed optimized functions for scalars
 *
 *  While the other functions use a memcpy to retrive the content into the
 *  user provided area, those ones use a regular affectation. It only works
 *  for scalar values, but should be a little faster.
 *
 *  @{
 */

  /** @brief Quick retrieval of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_get_as(dynar,idx,type) \
          *(type*)xbt_dynar_get_ptr(dynar,idx)
  /** @brief Quick insertion of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_insert_at_as(dynar,idx,type,value) \
          *(type*)xbt_dynar_insert_at_ptr(dynar,idx)=value
  /** @brief Quick insertion of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_push_as(dynar,type,value) \
          *(type*)xbt_dynar_push_ptr(dynar)=value
  /** @brief Quick insertion of scalar content 
   *  @hideinitializer */
#  define xbt_dynar_pop_as(dynar,type) \
           *(type*)xbt_dynar_pop_ptr(dynar)

/** @} */
/** @name 5. Cursors on DynArr
 *
 * Cursors are used to iterate over the structure. Never add elements to the 
 * DynArr during the traversal. To remove elements, use the
 * xbt_dynar_cursor_rm() function
 *
 *  @{
 */

  void xbt_dynar_cursor_first (const xbt_dynar_t dynar, int *cursor);
  void xbt_dynar_cursor_step  (const xbt_dynar_t dynar, int *cursor);
  int  xbt_dynar_cursor_get   (const xbt_dynar_t dynar, int *cursor, 
			       void *whereto);
  void xbt_dynar_cursor_rm(xbt_dynar_t dynar,
			   int          *const cursor);


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
int cpt;
string *str;
xbt_dynar_foreach (dyn,cpt,str) {
  printf("Seen %s\n",str);
}
\endcode
 */
#define xbt_dynar_foreach(_dynar,_cursor,_data) \
       for (xbt_dynar_cursor_first(_dynar,&(_cursor))      ; \
	    xbt_dynar_cursor_get(_dynar,&(_cursor),&_data) ; \
            xbt_dynar_cursor_step(_dynar,&(_cursor))         )


SG_END_DECL()

/* @} */
#endif /* _XBT_DYNAR_H */
