/* $Id$ */

/* dynar - a generic dynamic array                                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _GRAS_DYNAR_H
#define _GRAS_DYNAR_H

typedef struct gras_dynar_s gras_dynar_t;

/* pointer to a function freeing something */
typedef   void (void_f_ppvoid_t)(void**);
typedef   void (void_f_pvoid_t) (void*);

void          gras_dynar_new(gras_dynar_t **whereto, 
			    size_t elm_size, 
			    void_f_pvoid_t *free_func);
void          gras_dynar_free(gras_dynar_t *dynar);
void          gras_dynar_free_container(gras_dynar_t *dynar);

unsigned long gras_dynar_length(const gras_dynar_t *dynar);
void          gras_dynar_reset(gras_dynar_t *dynar);


/* regular array functions */
void gras_dynar_get(const gras_dynar_t *dynar,
		    int idx, void *dst);
void gras_dynar_set(gras_dynar_t *dynar,
		    int  idx, const void *src);
void gras_dynar_remplace(gras_dynar_t *dynar,
			 int  idx, const void *object);

/* perl array function */
void gras_dynar_insert_at(gras_dynar_t *dynar,
			  int  idx, const void *src);
void gras_dynar_remove_at(gras_dynar_t *dynar,
			  int  idx, void *object);
void gras_dynar_push     (gras_dynar_t *dynar, const void *src);
void gras_dynar_pop      (gras_dynar_t *dynar, void *dst);
void gras_dynar_unshift  (gras_dynar_t *dynar, const void *src);
void gras_dynar_shift    (gras_dynar_t *dynar, void *dst);
void gras_dynar_map      (const gras_dynar_t *dynar, void_f_pvoid_t *operator);

/* cursor functions */
void gras_dynar_cursor_first (const gras_dynar_t *dynar, int *cursor);
void gras_dynar_cursor_step  (const gras_dynar_t *dynar,
			      int  *cursor);
int  gras_dynar_cursor_get   (const gras_dynar_t *dynar,
			      int  *cursor, void *whereto);

/**
 * gras_dynar_foreach:
 * @_dynar: what to iterate over
 * @_cursor: an integer used as cursor
 * @_data:
 *
 * Iterates over the whole dynar. Example:
 *
 * <programlisting>
 * gras_dynar_t *dyn;
 * int cpt;
 * string *str;
 * gras_dynar_foreach (dyn,cpt,str) {
 *   printf("Seen %s\n",str);
 * }</programlisting>
 */
#define gras_dynar_foreach(_dynar,_cursor,_data) \
       for (gras_dynar_cursor_first(_dynar,&(_cursor))      ; \
	    gras_dynar_cursor_get(_dynar,&(_cursor),&_data) ; \
            gras_dynar_cursor_step(_dynar,&(_cursor))         )
/*
       for (gras_dynar_length(_dynar) && (_gras_dynar_cursor_first(_dynar,&_cursor),      \
					  1);     \
	    gras_dynar_length(_dynar) && gras_dynar_cursor_get(_dynar,&_cursor,&_data); \
            gras_dynar_cursor_step(_dynar,&_cursor))
*/
void gras_dynar_cursor_rm(gras_dynar_t * dynar,
			  int          * const cursor);

#endif /* _GRAS_DYNAR_H */
