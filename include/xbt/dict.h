/* $Id$ */

/* xbt/dict.h -- api to a generic dictionary                                */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _XBT_DICT_H
#define _XBT_DICT_H

#include "xbt/misc.h" /* BEGIN_DECL */
#include "xbt/error.h"

BEGIN_DECL()

/** @addtogroup XBT_dict
 * 
 *  This section describes the API to a dictionnary structure that
 *  associates as string to a void* key. Even if it provides the same
 *  functionnality than an hash table, the implementation differs and the
 *  internal data-structure rather looks like a tree.
 * 
 *  Here is a little example of use:
 *  \verbatim xbt_dict_t mydict = xbt_dict_new();
 char buff[512];

 sprintf(buff,"some very precious data");
 xbt_dict_set(mydict,"my data", strdump(buff), free); 

 sprintf(buff,"another good stuff");
 xbt_dict_set(mydict,"my data", strdup(buff), free); // previous data gets erased (and freed) by second add \endverbatim

 *
 * \warning This section also gets bitten by the doxygen bug reordering the name sections. 
 * Make sure to read in this order:
 *  -# Constructor/destructor
 *  -# Dictionnaries basic usage
 *  -# Non-null terminated keys
 *  -# Traversing dictionnaries with cursors
 *
 * @{
*/

/**  @name 1. Constructor/destructor
 *   @{
 */

  /** \brief Dictionnary data type (opaque structure) */
  typedef struct xbt_dict_ *xbt_dict_t;
  xbt_dict_t xbt_dict_new(void);
  void xbt_dict_free(xbt_dict_t *dict);

/** @} */
/** @name 2. Dictionnaries basic usage
 *
 * Careful, those functions assume that the key is null-terminated.
 *
 *  @{ */

  void xbt_dict_set(xbt_dict_t head, const char *key, void *data, void_f_pvoid_t *free_ctn);
  xbt_error_t xbt_dict_get(xbt_dict_t head,const char *key, /* OUT */void **data);
  xbt_error_t xbt_dict_remove(xbt_dict_t head,const char *key);
  void xbt_dict_dump(xbt_dict_t head,void (*output)(void*));
  
/** @} */
/** @name 3. Non-null terminated keys
 *
 * Those functions work even with non-null terminated keys.
 *
 *  @{
 */
  void xbt_dict_set_ext(xbt_dict_t     head,
		        const char     *key, int  key_len,
		        void           *data,
		        void_f_pvoid_t *free_ctn);
  xbt_error_t xbt_dict_get_ext(xbt_dict_t head,const char *key, int key_len,
			       /* OUT */void **data);
  xbt_error_t xbt_dict_remove_ext(xbt_dict_t head,
				  const char *key, int key_len);


/** @} */
/** @name 4. Cursors on dictionnaries 
 *
 *  Don't get impressed, there is a lot of functions here, but traversing a 
 *  dictionnary is imediate with the xbt_dict_foreach macro.
 *  You only need the other functions in rare cases (they are not used directly in SG itself).
 *
 *  Here is an example (assuming that the dictionnary contains strings, ie
 *  that the <tt>data</tt> argument of xbt_dict_set was always a null-terminated char*):
\verbatim xbt_dict_cursor_t cursor=NULL;
 char *key,*data;

 xbt_dict_foreach(head,cursor,key,data) {
    printf("   - Seen:  %s->%s\n",key,data);
 }\endverbatim

 *
 *  \warning Do not add or remove entries to the cache while traversing !!
 *
 *  @{ */

  /** \brief Cursor on dictionnaries (opaque type) */
  typedef struct xbt_dict_cursor_ *xbt_dict_cursor_t;
  xbt_dict_cursor_t xbt_dict_cursor_new(const xbt_dict_t head);
  void               xbt_dict_cursor_free(xbt_dict_cursor_t *cursor);

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
  /** \brief toto 
      \hideinitializer */
#  define xbt_dict_foreach(dict,cursor,key,data)                       \
    for (cursor=NULL, xbt_dict_cursor_first((dict),&(cursor)) ;        \
         xbt_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data));\
         xbt_dict_cursor_step(cursor) )

/** @} */
/** @} */

#if 0

                         MULTI-DICTS ARE BROKEN


/*####[ Multi cache functions ]##############################################*/
/* The are cache of cache of data. Any function there works the same way     */
/*  than their simple cache counterpart.                                     */
/*###############################"###########################################*/
/* To dump multicache, this function dumps a cache                           */
void xbt_dict_print(void *data);
/* To dump multicache, this one dumps a string                               */
void xbt_dict_prints(void *data);

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
#endif

END_DECL()

#endif /* _XBT_DICT_H */
