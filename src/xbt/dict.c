/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen() */

#include <stdio.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dict,xbt,
   "Dictionaries provide the same functionnalities than hash tables");

/*####[ Private prototypes ]#################################################*/


/*####[ Code ]###############################################################*/

/**
 * xbt_dict_new:
 *
 * @whereto: pointer to the destination
 *
 * Creates and initialize a new dictionnary
 */
xbt_dict_t 
xbt_dict_new(void) {
  xbt_dict_t res= xbt_new(s_xbt_dict_t,1);
  res->head=NULL;
  return res;
}
/**
 * xbt_dict_free:
 * @dict: the dictionnary to be freed
 *
 * Frees a cache structure with all its childs.
 */
void
xbt_dict_free(xbt_dict_t *dict)  {
  if (dict && *dict) {
    if ((*dict)->head) {
      xbt_dictelm_free( &( (*dict)->head ) );
      (*dict)->head = NULL;
    }
    xbt_free(*dict);
    *dict=NULL;
  }
}

/**
 * xbt_dict_set_ext:
 *
 * @dict: the container
 * @key: the key to set the new data
 * @data: the data to add in the dict
 *
 * set the @data in the structure under the @key, which can be any kind 
 * of data, as long as its length is provided in @key_len.
 */
void
xbt_dict_set_ext(xbt_dict_t      dict,
		  const char      *key,
		  int              key_len,
		  void            *data,
		  void_f_pvoid_t  *free_ctn) {

  xbt_assert(dict);

  xbt_dictelm_set_ext(&(dict->head),
		       key, key_len, data, free_ctn);
}

/**
 * xbt_dict_set:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 *
 * set the @data in the structure under the @key, which is a 
 * null terminated string.
 */
void
xbt_dict_set(xbt_dict_t     dict,
	      const char     *key,
	      void           *data,
	      void_f_pvoid_t *free_ctn) {

  xbt_assert(dict);
  
  xbt_dictelm_set(&(dict->head), key, data, free_ctn);
}

/**
 * xbt_dict_get_ext:
 *
 * @dict: the dealer of data
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: xbt_error
 *
 * Search the given @key. mismatch_error when not found.
 */
xbt_error_t
xbt_dict_get_ext(xbt_dict_t     dict,
		  const char     *key,
		  int             key_len,
		  /* OUT */void **data) {

  xbt_assert(dict);

  return xbt_dictelm_get_ext(dict->head, key, key_len, data);
}

/**
 * xbt_dict_get:
 *
 * @dict: the dealer of data
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: xbt_error
 *
 * Search the given @key. mismatch_error when not found.
 */
xbt_error_t
xbt_dict_get(xbt_dict_t     dict,
	      const char     *key,
	      /* OUT */void **data) {
  xbt_assert(dict);

  return xbt_dictelm_get(dict->head, key, data);
}


/**
 * xbt_dict_remove_ext:
 *
 * @dict: the trash can
 * @key: the key of the data to be removed
 * @Returns: xbt_error_t
 *
 * Remove the entry associated with the given @key
 */
xbt_error_t
xbt_dict_remove_ext(xbt_dict_t  dict,
                     const char  *key,
                     int          key_len) {
  xbt_assert(dict);
  
  return xbt_dictelm_remove_ext(dict->head, key, key_len);
}

/**
 * xbt_dict_remove:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: xbt_error_t
 *
 * Remove the entry associated with the given @key
 */
xbt_error_t
xbt_dict_remove(xbt_dict_t  dict,
		 const char  *key) {
  if (!dict) 
     RAISE1(mismatch_error,"Asked to remove key %s from NULL dict",key);

  return xbt_dictelm_remove(dict->head, key);
}


/**
 * xbt_dict_dump:
 *
 * @dict: the exibitionist
 * @output: a function to dump each data in the tree
 * @Returns: xbt_error_t
 *
 * Ouputs the content of the structure. (for debuging purpose). @ouput is a
 * function to output the data. If NULL, data won't be displayed.
 */

void
xbt_dict_dump(xbt_dict_t     dict,
               void_f_pvoid_t *output) {

  printf("Dict %p:\n", (void*)dict);
  xbt_dictelm_dump(dict ? dict->head: NULL, output);
}

