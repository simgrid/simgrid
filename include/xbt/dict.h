/* $Id$ */

/* xbt/dict.h -- api to a generic dictionary                                */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _XBT_DICT_H
#define _XBT_DICT_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */
#include "xbt/dynar.h" /* void_f_pvoid_t */

SG_BEGIN_DECL()

/** @addtogroup XBT_dict
 *  @brief The dictionnary data structure (comparable to hash tables)
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
 xbt_dict_set(mydict,"my data", strdup(buff), free); 

 sprintf(buff,"another good stuff");
 xbt_dict_set(mydict,"my data", strdup(buff), free); // previous data gets erased (and freed) by second add \endverbatim

 *
 */

/** @defgroup XBT_dict_cons Dict constructor and destructor
 *  @ingroup XBT_dict
 *
 *  @{
 */

  /** \brief Dictionnary data type (opaque structure) */
  typedef struct xbt_dict_ *xbt_dict_t;
  xbt_dict_t xbt_dict_new(void);
  void xbt_dict_free(xbt_dict_t *dict);

/** @} */
/** @defgroup XBT_dict_basic Dictionnaries basic usage
 *  @ingroup XBT_dict
 *
 * Careful, those functions assume that the key is null-terminated.
 *
 *  @{
 */

  void  xbt_dict_set(xbt_dict_t head, const char *key, void *data, void_f_pvoid_t *free_ctn);
  void *xbt_dict_get(xbt_dict_t head,const char *key);
  void *xbt_dict_get_or_null(xbt_dict_t dict, const char *key);

  void xbt_dict_remove(xbt_dict_t head, const char *key);
  void xbt_dict_dump(xbt_dict_t head,void (*output)(void*));
  
/** @} */
/** @defgroup XBT_dict_nnul Dictionnaries with non-nul terminated keys
 *  @ingroup XBT_dict
 *
 * Those functions work even with non-null terminated keys.
 *
 *  @{
 */
  void  xbt_dict_set_ext(xbt_dict_t     head,
	 	         const char     *key, int  key_len,
		         void           *data,
		         void_f_pvoid_t *free_ctn);
  void *xbt_dict_get_ext(xbt_dict_t head,const char *key, int key_len);
  void xbt_dict_remove_ext(xbt_dict_t head, const char *key, int key_len);


/** @} */
/** @defgroup XBT_dict_curs Cursors on dictionnaries 
 *  @ingroup XBT_dict
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

  /** @brief Cursor on dictionnaries (opaque type) */
  typedef struct xbt_dict_cursor_ *xbt_dict_cursor_t;
  xbt_dict_cursor_t xbt_dict_cursor_new(const xbt_dict_t head);
  void               xbt_dict_cursor_free(xbt_dict_cursor_t *cursor);

  void xbt_dict_cursor_rewind(xbt_dict_cursor_t cursor);


  char * xbt_dict_cursor_get_key     (xbt_dict_cursor_t cursor);
  void * xbt_dict_cursor_get_data    (xbt_dict_cursor_t cursor);

  void xbt_dict_cursor_first (const xbt_dict_t   dict,
			     xbt_dict_cursor_t *cursor);
  void         xbt_dict_cursor_step        (xbt_dict_cursor_t  cursor);
  int          xbt_dict_cursor_get_or_free (xbt_dict_cursor_t *cursor,
					    char              **key,
					    void              **data);
  /** @def xbt_dict_foreach
      @hideinitializer */
#  define xbt_dict_foreach(dict,cursor,key,data)                       \
    for (cursor=NULL, xbt_dict_cursor_first((dict),&(cursor)) ;        \
         xbt_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data));\
         xbt_dict_cursor_step(cursor) )

/** @} */
/** @defgroup XBT_dict_multi Multi-level dictionnaries
 *  @ingroup XBT_dict
 *
 * They can be seen as dictionnary of multiple keys or as dictionnary of 
 * dictionnary of ... of data. Most of the functions here work the same way 
 * than their simple dictionnary counterpart.
 *
 * Note that there is no xbt_multidict_free neither xbt_multi_dict_new functions.
 * Use xbt_dict_free() and xbt_dict_new() instead.
 *
 * @{
 */

/** \brief To dump multicache, this function dumps a cache                           */
void xbt_dict_print(void *data);
/** \brief To dump multicache, this one dumps a string                               */
void xbt_dict_prints(void *data);


/*----[ xbt_multidict_set ]--------------------------------------------------*/
void
xbt_multidict_set(xbt_dict_t mdict,
                  xbt_dynar_t keys,
                  void *data,void (*free_ctn)(void*));
void
xbt_multidict_set_ext(xbt_dict_t mdict,
                      xbt_dynar_t keys, xbt_dynar_t lens,
                      void *data,void_f_pvoid_t *free_ctn);

/*----[ xbt_multidict_get ]--------------------------------------------------*/
void *xbt_multidict_get    (xbt_dict_t mdict, xbt_dynar_t keys);
void *xbt_multidict_get_ext(xbt_dict_t mdict, xbt_dynar_t keys, xbt_dynar_t lens);

/*----[ xbt_multidict_remove ]-----------------------------------------------*/
/*---------------------------------------------------------------------------*/
void xbt_multidict_remove(xbt_dict_t mdict, xbt_dynar_t keys);
void xbt_multidict_remove_ext(xbt_dict_t mdict, xbt_dynar_t keys, xbt_dynar_t lens);

/** @} */

SG_END_DECL()

#endif /* _XBT_DICT_H */
