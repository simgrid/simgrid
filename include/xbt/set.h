/* $Id$ */

/* xbt/set.h -- api to a generic dictionary                                 */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_SET_H
#define _XBT_SET_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

/** @addtogroup XBT_set
 *  @brief A data container consisting in \ref XBT_dict and \ref XBT_dynar
 * 
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref XBT_API]
 *                <tr><td><b>Prev</b>   <td> [\ref XBT_dict]
 *                <tr><td><b>Next</b>   <td> [\ref XBT_fifo]       
 *                <tr><td><b>Down</b>   <td> [\ref XBT_set_cons]        </table></center>
 *
 *  The elements stored in such a data structure can be retrieve both by
 *  name and by ID. For this to work, the first fields of the structures
 *  stored must begin with:
 *  \verbatim unsigned int ID;
 char        *name;
 unsigned int name_len;\endverbatim
 *
 *  It is impossible to remove an element from such a data structure.
 *
 *  @todo
 *  Such a datastructure was necessary/useful to store the GRAS type 
 *  descriptions, but it should be reworked to become generic.
 *
 */


/** @defgroup XBT_set_cons Set and set elements, constructor/destructor
 *  @ingroup XBT_set
 *
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref XBT_API]::[\ref XBT_set]
 *                <tr><td>   Prev       <td> 
 *                <tr><td><b>Next</b>   <td> [\ref XBT_set_basic]        </table></center>
 *
 *  @{
 */
/** \brief Opaque type representing a set */
typedef struct xbt_set_ *xbt_set_t; 
/** \brief It must be possible to cast set elements to this type */
struct xbt_set_elm_ {
  unsigned int ID;      /**< Identificator (system assigned) */
  char        *name;    /**< Name (user assigned) */
  unsigned int name_len;/**< Length of the name */
};

/*####[ Functions ]##########################################################*/
xbt_set_t xbt_set_new (void);
void xbt_set_free(xbt_set_t *set);

/** @} */
typedef struct xbt_set_elm_  s_xbt_set_elm_t;
typedef struct xbt_set_elm_ *  xbt_set_elm_t;
/** @defgroup XBT_set_basic Sets basic usage
 *  @ingroup XBT_set
 *
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref XBT_API]::[\ref XBT_set]
 *                <tr><td><b>Prev</b>   <td> [\ref XBT_set_cons]
 *                <tr><td><b>Next</b>   <td> [\ref XBT_set_curs]        </table></center>
 *
 *  @{
 */

void xbt_set_add (xbt_set_t set, xbt_set_elm_t  elm, void_f_pvoid_t *free_func);

xbt_set_elm_t xbt_set_get_by_name    (xbt_set_t set, const char *key);
xbt_set_elm_t xbt_set_get_by_name_ext(xbt_set_t set, const char *key, int key_len);
xbt_set_elm_t xbt_set_get_by_id      (xbt_set_t set, int         id);
				      
/** @} */
/** @defgroup XBT_set_curs Sets cursors
 *  @ingroup XBT_set
 *
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref XBT_API]::[\ref XBT_set]
 *                <tr><td><b>Prev</b>   <td> [\ref XBT_set_basic]
 *                <tr><td>   Next       <td>                       </table></center>
 *
 *  \warning Don't add or remove entries to the cache while traversing
 *
 *  @{
 */

/** @brief Cursor type */
typedef struct xbt_set_cursor_ *xbt_set_cursor_t;

void         xbt_set_cursor_first       (xbt_set_t         set,
					  xbt_set_cursor_t *cursor);
void         xbt_set_cursor_step        (xbt_set_cursor_t  cursor);
int          xbt_set_cursor_get_or_free (xbt_set_cursor_t *cursor,
					  xbt_set_elm_t    *elm);

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
