/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

#ifndef _XBT_SWAG_H
#define _XBT_SWAG_H

#include "xbt/sysdep.h"
#include "gras_config.h" /* inline */

/* This type should be added to a type that is to be used in such a swag */
/* Whenever a new object with this struct is created, all fields have
   to be set to NULL */

/** \name Swag types
    \ingroup XBT_swag

    Specific set. 

    These typedefs are public so that the compiler can
    do his job but believe me, you don't want to try to play with 
    those structs directly. Use them as an abstract datatype.
*/
/* @{ */
typedef struct xbt_swag_hookup {
  void *next;
  void *prev;
} s_xbt_swag_hookup_t; 
/**< This type should be added to a type that is to be used in a swag. 
 * For example like that :

\code
typedef struct foo {
  s_xbt_swag_hookup_t set1_hookup;
  s_xbt_swag_hookup_t set2_hookup;

  double value;
} s_foo_t, *foo_t;
...
{
  s_foo_t elem;
  xbt_swag_t set1=NULL;
  xbt_swag_t set2=NULL;

  set1 = xbt_swag_new(xbt_swag_offset(elem, set1_hookup));
  set2 = xbt_swag_new(xbt_swag_offset(elem, set2_hookup));

}
\endcode
*/
typedef s_xbt_swag_hookup_t  *xbt_swag_hookup_t;


typedef struct xbt_swag {
  void *head;
  void *tail;
  size_t offset;
  int count;
} s_xbt_swag_t, *xbt_swag_t;
/**< A typical swag */
/* @} */

xbt_swag_t xbt_swag_new(size_t offset);
void xbt_swag_free(xbt_swag_t swag);
void xbt_swag_init(xbt_swag_t swag, size_t offset);
void xbt_swag_insert(void *obj, xbt_swag_t swag);
void xbt_swag_insert_at_head(void *obj, xbt_swag_t swag);
void xbt_swag_insert_at_tail(void *obj, xbt_swag_t swag);
void *xbt_swag_remove(void *obj, xbt_swag_t swag);
void *xbt_swag_extract(xbt_swag_t swag);
int xbt_swag_size(xbt_swag_t swag);
int xbt_swag_belongs(void *obj, xbt_swag_t swag);

static inline void *xbt_swag_getFirst(xbt_swag_t swag)
{
  return (swag->head);
}

#define xbt_swag_getNext(obj,offset) (((xbt_swag_hookup_t)(((char *) (obj)) + (offset)))->prev)
#define xbt_swag_getPrev(obj,offset) (((xbt_swag_hookup_t)(((char *) (obj)) + (offset)))->next)

/** 
 * \ingroup XBT_swag
 * \brief Offset computation
 * \arg var a variable of type <tt>struct</tt> something
 * \arg field a field of <tt>struct</tt> something
 * \return the offset of \a field in <tt>struct</tt> something.
 *
 * It is very similar to offsetof except that is done at runtime and that 
 * you have to declare a variable. Why defining such a macro then ? 
 * Because it is portable...
 */
#define xbt_swag_offset(var,field) ((char *)&( (var).field ) - (char *)&(var))

/**
 \name Swag iterator
 \ingroup XBT_swag
 *
 * Iterates over the whole swag. 
 */
/* @{ */
#define xbt_swag_foreach(obj,swag)                            \
   for((obj)=xbt_swag_getFirst((swag));                       \
       (obj)!=NULL;                                           \
       (obj)=xbt_swag_getNext((obj),(swag)->offset))
/**<  A simple swag iterator 
 * \param obj the indice of the loop
 * \param swag what to iterate over

    \warning you cannot modify the \a swag while using this loop  */
#define xbt_swag_foreach_safe(obj,obj_next,swag)                  \
   for((obj)=xbt_swag_getFirst((swag)),                           \
       ((obj)?(obj_next=xbt_swag_getNext((obj),(swag)->offset)):  \
	         (obj_next=NULL));                                \
       (obj)!=NULL;                                               \
       (obj)=obj_next,                                            \
       ((obj)?(obj_next=xbt_swag_getNext((obj),(swag)->offset)):  \
                 (obj_next=NULL))     )
/**< A safe swag iterator 
 * \param obj the indice of the loop
 * \param obj_next the object that is right after (if any) \a obj in the swag
 * \param swag what to iterate over

    You can safely modify the \a swag while using this loop. 
    Well, safely... Err. You can remove \a obj without having any 
    trouble at least.  */
/* @} */

#endif    /* _XBT_SWAG_H */
