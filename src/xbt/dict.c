/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras_private.h"
#include "dict_private.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen() */

#include <stdio.h>

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(dict,gros,
   "Dictionaries provide the same functionnalities than hash tables");

/*####[ Private prototypes ]#################################################*/


/*####[ Code ]###############################################################*/

/**
 * gras_dict_new:
 *
 * @whereto: pointer to the destination
 *
 * Creates and initialize a new dictionnary
 */
gras_error_t
gras_dict_new(gras_dict_t **whereto) {
  gras_dict_t *dict;

  if (!(dict = calloc(1, sizeof(gras_dict_t))))
    RAISE_MALLOC;

  dict->head=NULL;

  *whereto = dict;
  
  return no_error;
}
/**
 * gras_dict_free:
 * @dict: the dictionnary to be freed
 *
 * Frees a cache structure with all its childs.
 */
void
gras_dict_free(gras_dict_t **dict)  {
  if (dict && *dict) {
    if ((*dict)->head) {
      gras_dictelm_free( &( (*dict)->head ) );
      (*dict)->head = NULL;
    }
    free(*dict);
    *dict=NULL;
  }
}

/**
 * gras_dict_set_ext:
 *
 * @p_dict: the container
 * @key: the key to set the new data
 * @data: the data to add in the dict
 * @Returns: a gras_error
 *
 * set the @data in the structure under the @key, which can be any kind 
 * of data, as long as its length is provided in @key_len.
 */
gras_error_t
gras_dict_set_ext(gras_dict_t     *p_dict,
		     const char      *key,
		     int              key_len,
		     void            *data,
		     void_f_pvoid_t  *free_ctn) {

  gras_assert(p_dict);

  return gras_dictelm_set_ext(&(p_dict->head),
				 key, key_len, data, free_ctn);
}

/**
 * gras_dict_set:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 * @Returns: a gras_error
 *
 * set the @data in the structure under the @key, which is a 
 * null terminated string.
 */
gras_error_t
gras_dict_set(gras_dict_t    *p_dict,
		 const char     *key,
		 void           *data,
		 void_f_pvoid_t *free_ctn) {

  gras_assert(p_dict);
  
  return gras_dictelm_set(&(p_dict->head), key, data, free_ctn);
}

/**
 * gras_dict_get_ext:
 *
 * @dict: the dealer of data
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: gras_error
 *
 * Search the given @key. mismatch_error when not found.
 */
gras_error_t
gras_dict_get_ext(gras_dict_t    *dict,
		       const char     *key,
		       int             key_len,
		       /* OUT */void **data) {

  gras_assert(dict);

  return gras_dictelm_get_ext(dict->head, key, key_len, data);
}

/**
 * gras_dict_get:
 *
 * @dict: the dealer of data
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: gras_error
 *
 * Search the given @key. mismatch_error when not found.
 */
gras_error_t
gras_dict_get(gras_dict_t    *dict,
                   const char     *key,
                   /* OUT */void **data) {
  gras_assert(dict);

  return gras_dictelm_get(dict->head, key, data);
}


/**
 * gras_dict_remove_ext:
 *
 * @dict: the trash can
 * @key: the key of the data to be removed
 * @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 */
gras_error_t
gras_dict_remove_ext(gras_dict_t *dict,
                     const char  *key,
                     int          key_len) {
  gras_assert(dict);
  
  return gras_dictelm_remove_ext(dict->head, key, key_len);
}

/**
 * gras_dict_remove:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 */
gras_error_t
gras_dict_remove(gras_dict_t *dict,
		 const char  *key) {
  if (!dict) 
     RAISE1(mismatch_error,"Asked to remove key %s from NULL dict",key);

  return gras_dictelm_remove(dict->head, key);
}


/**
 * gras_dict_dump:
 *
 * @dict: the exibitionist
 * @output: a function to dump each data in the tree
 * @Returns: gras_error_t
 *
 * Ouputs the content of the structure. (for debuging purpose). @ouput is a
 * function to output the data. If NULL, data won't be displayed.
 */

gras_error_t
gras_dict_dump(gras_dict_t    *dict,
               void_f_pvoid_t *output) {

  printf("Dict %p:\n", (void*)dict);
  return gras_dictelm_dump(dict ? dict->head: NULL, output);
}

