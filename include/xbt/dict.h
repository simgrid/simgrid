/* $Id$ */

/* gras/dict.h -- api to a generic dictionary                               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _GRAS_DICT_H
#define _GRAS_DICT_H

#ifdef  __cplusplus
extern "C" 
#endif

/*####[ Type definition ]####################################################*/
typedef struct gras_dict_ gras_dict_t;

/*####[ Simple dict  functions ]#############################################*/

gras_dict_t *gras_dict_new(void);
void gras_dict_free(gras_dict_t **dict);


void gras_dict_set    (gras_dict_t    *head,
		       const char     *key,
		       void           *data,
		       void_f_pvoid_t *free_ctn);
void gras_dict_set_ext(gras_dict_t    *head,
		       const char     *key,
		       int             key_len,
		       void           *data,
		       void_f_pvoid_t *free_ctn);

/*----[ gras_dict_get ]------------------------------------------------------*/
/* Search the given #key#. data=NULL when not found.                         */
/* Returns true if anything went ok, and false on internal error.            */
/*---------------------------------------------------------------------------*/
gras_error_t gras_dict_get(gras_dict_t *head,const char *key,
			   /* OUT */void **data);
gras_error_t gras_dict_get_ext(gras_dict_t *head,const char *key,
			       int key_len,
			       /* OUT */void **data);
/*----[ gras_dict_remove ]---------------------------------------------------*/
/* Remove the entry associated with the given #key#.                         */
/* Returns if ok. Removing a non-existant key is ok.                         */
/*---------------------------------------------------------------------------*/
gras_error_t gras_dict_remove(gras_dict_t *head,const char *key);

gras_error_t gras_dict_remove_ext(gras_dict_t *head,const char *key,
                                  int key_len);

/*----[ gras_dict_dump ]-----------------------------------------------------*/
/* Outputs the content of the structure. (for debuging purpose)              */
/* #output# is a function to output the data.If NULL, data won't be displayed*/
/*---------------------------------------------------------------------------*/
void gras_dict_dump(gras_dict_t *head,
		    void (*output)(void*));
/*----[ gras_dict_print ]----------------------------------------------------*/
/* To dump multicache, this function dump a cache                            */
/*---------------------------------------------------------------------------*/
void gras_dict_print(void *data);
/* To dump multicache, this one dumps a string                               */
void gras_dict_prints(void *data);


/*####[ Multi cache functions ]##############################################*/
/* The are cache of cache of data. Any function there works the same way     */
/*  than their simple cache counterpart.                                     */
/*###############################"###########################################*/

/*----[ gras_multidict_free ]------------------------------------------------*/
/* This function does not exist. Use gras_dict_free instead.                 */
/*---------------------------------------------------------------------------*/

/*----[ gras_multidict_set ]-------------------------------------------------*/
/* Insert the data in the structure under the #keycount# #key#s.             */
/* The key are destroyed in the process. Think to strdup it before.          */
/* Returns if it was ok or not                                               */
/*---------------------------------------------------------------------------*/
gras_error_t gras_multidict_set(gras_dict_t **head,
				int keycount,char **key,
				void *data,void (*free_ctn)(void*));

gras_error_t gras_multidict_set_ext(gras_dict_t **head,
				    int keycount,char **key,int *key_len,
				    void *data,void_f_pvoid_t *free_ctn);

/*----[ gras_multidict_get ]-------------------------------------------------*/
/* Search the given #key#. data=NULL when not found.                         */
/* Returns true if anything went ok, and false on internal error.            */
/*---------------------------------------------------------------------------*/
gras_error_t gras_multidict_get(gras_dict_t *head,
				int keycount,const char **key,
				/* OUT */void **data);

gras_error_t gras_multidict_get_ext(gras_dict_t *head,
				    int keycount,const char **key,int *key_len,
				    /* OUT */void **data);

/*----[ gras_multidict_remove ]----------------------------------------------*/
/* Remove the entry associated with the given #key#.                         */
/* Returns if ok. Removing a non-existant key is ok.                         */
/*---------------------------------------------------------------------------*/
gras_error_t gras_multidict_remove(gras_dict_t *head,
				   int keycount,const char **key);

gras_error_t gras_multidict_remove_ext(gras_dict_t *head,
                                       int keycount,const char **key,int *key_len);

/*####[ Cache cursor functions ]#############################################*/
/* To traverse (simple) caches                                               */
/* Don't add or remove entries to the cache while traversing !!!             */
/*###########################################################################*/
typedef struct gras_dict_cursor_ gras_dict_cursor_t;
/* creator/destructor */
gras_dict_cursor_t *gras_dict_cursor_new(const gras_dict_t *head);
void                gras_dict_cursor_free(gras_dict_cursor_t *cursor);

/* back to first element 
   it is not enough to reinit the cache after an add/remove in cache*/
void gras_dict_cursor_rewind(gras_dict_cursor_t *cursor);


gras_error_t gras_dict_cursor_get_key     (gras_dict_cursor_t *cursor,
					   /*OUT*/char **key);
gras_error_t gras_dict_cursor_get_data    (gras_dict_cursor_t *cursor,
					   /*OUT*/void **data);

void gras_dict_cursor_first (const gras_dict_t   *dict,
			     gras_dict_cursor_t **cursor);
void         gras_dict_cursor_step        (gras_dict_cursor_t  *cursor);
int          gras_dict_cursor_get_or_free (gras_dict_cursor_t **cursor,
					   char               **key,
					   void               **data);
#define gras_dict_foreach(dict,cursor,key,data)                        \
  for (cursor=NULL, gras_dict_cursor_first((dict),&(cursor)) ;         \
       gras_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data)); \
       gras_dict_cursor_step(cursor) )

#ifdef  __cplusplus
}
#endif

#endif /* _GRAS_DICT_H */
