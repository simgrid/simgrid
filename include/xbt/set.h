/* $Id$ */

/* gras/set.h -- api to a generic dictionary                                */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _GRAS_SET_H
#define _GRAS_SET_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL

/*####[ Type definition ]####################################################*/
typedef struct gras_set_ *gras_set_t;
typedef struct gras_set_elm_ {
  unsigned int ID;
  char        *name;
  unsigned int name_len;
} s_gras_set_elm_t,*gras_set_elm_t;

/*####[ Functions ]##########################################################*/

gras_set_t gras_set_new (void);
void gras_set_free(gras_set_t *set);


void gras_set_add (gras_set_t      set,
		   gras_set_elm_t  elm,
		   void_f_pvoid_t *free_func);

/*----[ gras_set_retrieve ]-------------------------------------------------*/
/* Search the given #key#. data=NULL when not found.                         */
/*---------------------------------------------------------------------------*/
gras_error_t gras_set_get_by_name    (gras_set_t      set,
				      const char     *key,
				      /* OUT */gras_set_elm_t *dst);
gras_error_t gras_set_get_by_name_ext(gras_set_t      set,
				      const char     *name,
				      int             name_len,
				      /* OUT */gras_set_elm_t *dst);
gras_error_t gras_set_get_by_id      (gras_set_t      set,
				      int             id,
				      /* OUT */gras_set_elm_t *dst);
				      
/*####[ Cache cursor functions ]#############################################*/
/* To traverse (simple) caches                                               */
/* Don't add or remove entries to the cache while traversing !!!             */
/*###########################################################################*/
typedef struct gras_set_cursor_ *gras_set_cursor_t;

void         gras_set_cursor_first       (gras_set_t         set,
					  gras_set_cursor_t *cursor);
void         gras_set_cursor_step        (gras_set_cursor_t  cursor);
int          gras_set_cursor_get_or_free (gras_set_cursor_t *cursor,
					  gras_set_elm_t    *elm);

#define gras_set_foreach(set,cursor,elm)                       \
  for ((cursor) = NULL, gras_set_cursor_first((set),&(cursor)) ;   \
       gras_set_cursor_get_or_free(&(cursor),(gras_set_elm_t*)&(elm));          \
       gras_set_cursor_step(cursor) )

END_DECL

#endif /* _GRAS_SET_H */
