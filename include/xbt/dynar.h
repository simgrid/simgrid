/* $Id$ */

/* dynar - a generic dynamic array                                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DYNAR_H
#define _XBT_DYNAR_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL

typedef struct xbt_dynar_s *xbt_dynar_t;

xbt_dynar_t  xbt_dynar_new(unsigned long elm_size, 
			     void_f_pvoid_t *free_func);
void          xbt_dynar_free(xbt_dynar_t *dynar);
void          xbt_dynar_free_container(xbt_dynar_t *dynar);

unsigned long xbt_dynar_length(const xbt_dynar_t dynar);
void          xbt_dynar_reset(xbt_dynar_t dynar);


/* regular array functions */
void xbt_dynar_get_cpy(const xbt_dynar_t dynar, int idx, void *const dst);
void *xbt_dynar_get_ptr(const xbt_dynar_t dynar,
			 const int          idx);

#define xbt_dynar_get_as(dynar,idx,type) *(type*)xbt_dynar_get_ptr(dynar,idx)
  
void xbt_dynar_set(xbt_dynar_t dynar, int  idx, const void *src);
void xbt_dynar_replace(xbt_dynar_t dynar,
			int idx, const void *object);

/* perl array function */

void xbt_dynar_insert_at(xbt_dynar_t dynar,
			  int  idx, const void *src);
void xbt_dynar_remove_at(xbt_dynar_t dynar,
			  int  idx, void *object);
void xbt_dynar_push     (xbt_dynar_t dynar, const void *src);
void xbt_dynar_pop      (xbt_dynar_t dynar, void *const dst);
void xbt_dynar_unshift  (xbt_dynar_t dynar, const void *src);
void xbt_dynar_shift    (xbt_dynar_t dynar, void *const dst);
void xbt_dynar_map      (const xbt_dynar_t dynar, void_f_pvoid_t *operator);

/* speed-optimized versions */
void *xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar,
			       const int          idx);
void *xbt_dynar_push_ptr(xbt_dynar_t dynar);
void *xbt_dynar_pop_ptr(xbt_dynar_t dynar);

#define xbt_dynar_insert_at_as(dynar,idx,type,value) *(type*)xbt_dynar_insert_at_ptr(dynar,idx)=value
#define xbt_dynar_push_as(dynar,type,value) *(type*)xbt_dynar_push_ptr(dynar)=value
#define xbt_dynar_pop_as(dynar,type) *(type*)xbt_dynar_pop_ptr(dynar)


/* cursor functions */
void xbt_dynar_cursor_first (const xbt_dynar_t dynar, int *cursor);
void xbt_dynar_cursor_step  (const xbt_dynar_t dynar, int *cursor);
int  xbt_dynar_cursor_get   (const xbt_dynar_t dynar, int *cursor, void *whereto);

/**
 * xbt_dynar_foreach:
 * @_dynar: what to iterate over
 * @_cursor: an integer used as cursor
 * @_data:
 *
 * Iterates over the whole dynar. Example:
 *
 * <programlisting>
 * xbt_dynar_t dyn;
 * int cpt;
 * string *str;
 * xbt_dynar_foreach (dyn,cpt,str) {
 *   printf("Seen %s\n",str);
 * }</programlisting>
 */
#define xbt_dynar_foreach(_dynar,_cursor,_data) \
       for (xbt_dynar_cursor_first(_dynar,&(_cursor))      ; \
	    xbt_dynar_cursor_get(_dynar,&(_cursor),&_data) ; \
            xbt_dynar_cursor_step(_dynar,&(_cursor))         )
/*
       for (xbt_dynar_length(_dynar) && (_xbt_dynar_cursor_first(_dynar,&_cursor),      \
					  1);     \
	    xbt_dynar_length(_dynar) && xbt_dynar_cursor_get(_dynar,&_cursor,&_data); \
            xbt_dynar_cursor_step(_dynar,&_cursor))
*/
void xbt_dynar_cursor_rm(xbt_dynar_t dynar,
			  int          *const cursor);

END_DECL
#endif /* _XBT_DYNAR_H */
