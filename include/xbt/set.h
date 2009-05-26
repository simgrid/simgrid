/* $Id$ */

/* xbt/set.h -- api to a generic dictionary                                 */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_SET_H
#define _XBT_SET_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_set
 *  @brief A data container consisting in \ref XBT_dict and \ref XBT_dynar
 * 
 *  The elements stored in such a data structure can be retrieve both by
 *  name and by ID. For this to work, the first fields of the structures
 *  stored must begin with the following fields:
 *  \verbatim struct {
 unsigned int ID;
 char        *name;
 unsigned int name_len;
 // my other fields, constituting the payload 
} my_element_type_t; \endverbatim
 * 
 *  Since we are casting elements around, no protection is ensured by the 
 * compiler. It is thus safer to define the headers using the macro 
 * defined to that extend:
 *  \verbatim struct {
 XBT_SET_HEADERS;

 // my other fields, constituting the payload 
} my_element_type_t; \endverbatim
 *
 *  It is now possible to remove an element from such a data structure.
 *
 *  @todo
 *  Such a datastructure was necessary/useful to store the GRAS type 
 *  descriptions, but it should be reworked to become generic.
 *
 */
/** @defgroup XBT_set_cons Set and set elements, constructor/destructor
 *  @ingroup XBT_set
 *
 *  @{
 */
/** \brief Opaque type representing a set */
     typedef struct xbt_set_ *xbt_set_t;

#define XBT_SET_HEADERS \
  unsigned int ID;      \
  char        *name;    \
  unsigned int name_len

/** \brief It must be possible to cast set elements to this type */
     typedef struct xbt_set_elm_ {
       unsigned int ID; /**< Identificator (system assigned) */
       char *name;      /**< Name (user assigned) */
       unsigned int name_len;
                        /**< Length of the name */
     } s_xbt_set_elm_t, *xbt_set_elm_t;

/*####[ Functions ]##########################################################*/
XBT_PUBLIC(xbt_set_t) xbt_set_new(void);
XBT_PUBLIC(void) xbt_set_free(xbt_set_t * set);

/** @} */
/** @defgroup XBT_set_basic Sets basic usage
 *  @ingroup XBT_set
 *
 *  @{
 */

XBT_PUBLIC(void) xbt_set_add(xbt_set_t set, xbt_set_elm_t elm,
                             void_f_pvoid_t free_func);
XBT_PUBLIC(void) xbt_set_remove(xbt_set_t set, xbt_set_elm_t elm);
XBT_PUBLIC(void) xbt_set_remove_by_name(xbt_set_t set, const char *key);
XBT_PUBLIC(void) xbt_set_remove_by_name_ext(xbt_set_t set, const char *key,
                                            int key_len);
XBT_PUBLIC(void) xbt_set_remove_by_id(xbt_set_t set, int id);

XBT_PUBLIC(xbt_set_elm_t) xbt_set_get_by_name(xbt_set_t set, const char *key);
XBT_PUBLIC(xbt_set_elm_t) xbt_set_get_by_name_ext(xbt_set_t set,
                                                  const char *key,
                                                  int key_len);
XBT_PUBLIC(xbt_set_elm_t) xbt_set_get_by_id(xbt_set_t set, int id);

XBT_PUBLIC(unsigned long) xbt_set_length(const xbt_set_t set);


/** @} */
/** @defgroup XBT_set_curs Sets cursors
 *  @ingroup XBT_set
 *
 *  \warning Don't add or remove entries to the cache while traversing
 *
 *  @{
 */

/** @brief Cursor type */
     typedef struct xbt_set_cursor_ *xbt_set_cursor_t;

XBT_PUBLIC(void) xbt_set_cursor_first(xbt_set_t set,
                                      xbt_set_cursor_t * cursor);
XBT_PUBLIC(void) xbt_set_cursor_step(xbt_set_cursor_t cursor);
XBT_PUBLIC(int) xbt_set_cursor_get_or_free(xbt_set_cursor_t * cursor,
                                           xbt_set_elm_t * elm);

/** @brief Iterates over the whole set
 *  @hideinitializer
 */
#define xbt_set_foreach(set,cursor,elm)                       \
  for ((cursor) = NULL, xbt_set_cursor_first((set),&(cursor)) ;   \
       xbt_set_cursor_get_or_free(&(cursor),(xbt_set_elm_t*)&(elm));          \
       xbt_set_cursor_step(cursor) )

/* @} */
SG_END_DECL()
#endif /* _XBT_SET_H */
