/* $Id$ */

/* xbt/dict.h -- api to a generic dictionary                               */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _XBT_DICT_H
#define _XBT_DICT_H

#include "xbt/misc.h" /* BEGIN_DECL */
#include "xbt/error.h" /* BEGIN_DECL */

#ifdef  __cplusplus
extern "C" 
#endif

/*####[ Type definition ]####################################################*/
typedef struct xbt_dict_ *xbt_dict_t;

/*####[ Simple dict  functions ]#############################################*/

xbt_dict_t xbt_dict_new(void);
void xbt_dict_free(xbt_dict_t *dict);


void xbt_dict_set    (xbt_dict_t     head,
		       const char     *key,
		       void           *data,
		       void_f_pvoid_t *free_ctn);
void xbt_dict_set_ext(xbt_dict_t     head,
		       const char     *key,
		       int             key_len,
		       void           *data,
		       void_f_pvoid_t *free_ctn);

/*----[ xbt_dict_get ]------------------------------------------------------*/
/* Search the given #key#. data=NULL when not found.                         */
/* Returns true if anything went ok, and false on internal error.            */
/*---------------------------------------------------------------------------*/
xbt_error_t xbt_dict_get(xbt_dict_t head,const char *key,
			   /* OUT */void **data);
xbt_error_t xbt_dict_get_ext(xbt_dict_t head,const char *key,
			       int key_len,
			       /* OUT */void **data);
/*----[ xbt_dict_remove ]---------------------------------------------------*/
/* Remove the entry associated with the given #key#.                         */
/* Returns if ok. Removing a non-existant key is ok.                         */
/*---------------------------------------------------------------------------*/
xbt_error_t xbt_dict_remove(xbt_dict_t head,const char *key);

xbt_error_t xbt_dict_remove_ext(xbt_dict_t head,
				  const char *key, int key_len);

/*----[ xbt_dict_dump ]-----------------------------------------------------*/
/* Outputs the content of the structure. (for debuging purpose)              */
/* #output# is a function to output the data.If NULL, data won't be displayed*/
/*---------------------------------------------------------------------------*/
void xbt_dict_dump(xbt_dict_t head,
		    void (*output)(void*));
/*----[ xbt_dict_print ]----------------------------------------------------*/
/* To dump multicache, this function dump a cache                            */
/*---------------------------------------------------------------------------*/
void xbt_dict_print(void *data);
/* To dump multicache, this one dumps a string                               */
void xbt_dict_prints(void *data);


/*####[ Multi cache functions ]##############################################*/
/* The are cache of cache of data. Any function there works the same way     */
/*  than their simple cache counterpart.                                     */
/*###############################"###########################################*/

/*----[ xbt_multidict_free ]------------------------------------------------*/
/* This function does not exist. Use xbt_dict_free instead.                 */
/*---------------------------------------------------------------------------*/

/*----[ xbt_multidict_set ]-------------------------------------------------*/
/* Insert the data in the structure under the #keycount# #key#s.             */
/* The key are destroyed in the process. Think to strdup it before.          */
/* Returns if it was ok or not                                               */
/*---------------------------------------------------------------------------*/
xbt_error_t xbt_multidict_set(xbt_dict_t *head,
				int keycount,char **key,
				void *data,void (*free_ctn)(void*));

xbt_error_t xbt_multidict_set_ext(xbt_dict_t *head,
				    int keycount,char **key,int *key_len,
				    void *data,void_f_pvoid_t *free_ctn);

/*----[ xbt_multidict_get ]-------------------------------------------------*/
/* Search the given #key#. data=NULL when not found.                         */
/* Returns true if anything went ok, and false on internal error.            */
/*---------------------------------------------------------------------------*/
xbt_error_t xbt_multidict_get(xbt_dict_t head,
				int keycount,const char **key,
				/* OUT */void **data);

xbt_error_t xbt_multidict_get_ext(xbt_dict_t head,
				    int keycount,const char **key,int *key_len,
				    /* OUT */void **data);

/*----[ xbt_multidict_remove ]----------------------------------------------*/
/* Remove the entry associated with the given #key#.                         */
/* Returns if ok. Removing a non-existant key is ok.                         */
/*---------------------------------------------------------------------------*/
xbt_error_t xbt_multidict_remove(xbt_dict_t head,
				   int keycount,const char **key);

xbt_error_t xbt_multidict_remove_ext(xbt_dict_t head,
                                       int keycount,const char **key,int *key_len);

/*####[ Cache cursor functions ]#############################################*/
/* To traverse (simple) caches                                               */
/* Don't add or remove entries to the cache while traversing !!!             */
/*###########################################################################*/
typedef struct xbt_dict_cursor_ *xbt_dict_cursor_t;
/* creator/destructor */
xbt_dict_cursor_t xbt_dict_cursor_new(const xbt_dict_t head);
void               xbt_dict_cursor_free(xbt_dict_cursor_t *cursor);

/* back to first element 
   it is not enough to reinit the cache after an add/remove in cache*/
void xbt_dict_cursor_rewind(xbt_dict_cursor_t cursor);


xbt_error_t xbt_dict_cursor_get_key     (xbt_dict_cursor_t cursor,
					   /*OUT*/char **key);
xbt_error_t xbt_dict_cursor_get_data    (xbt_dict_cursor_t cursor,
					   /*OUT*/void **data);

void xbt_dict_cursor_first (const xbt_dict_t   dict,
			     xbt_dict_cursor_t *cursor);
void         xbt_dict_cursor_step        (xbt_dict_cursor_t  cursor);
int          xbt_dict_cursor_get_or_free (xbt_dict_cursor_t *cursor,
					   char              **key,
					   void              **data);
#define xbt_dict_foreach(dict,cursor,key,data)                       \
  for (cursor=NULL, xbt_dict_cursor_first((dict),&(cursor)) ;        \
       xbt_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data));\
       xbt_dict_cursor_step(cursor) )

#ifdef  __cplusplus
}
#endif

#endif /* _XBT_DICT_H */
