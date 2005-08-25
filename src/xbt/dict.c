/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Copyright (c) 2003,2004 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "dict_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dict,xbt,
   "Dictionaries provide the same functionnalities than hash tables");
/*####[ Private prototypes ]#################################################*/

/*####[ Code ]###############################################################*/

/**
 * @brief Constructor
 * @return pointer to the destination
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
 * @brief Destructor
 * @param dict the dictionnary to be freed
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
    free(*dict);
    *dict=NULL;
  }
}

/**
 * \brief Add data to the dict (arbitrary key)
 * \param dict the container
 * \param key the key to set the new data
 * \param key_len the size of the \a key
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a key as argument) when 
 *        \a key is removed from the dictionnary
 *
 * set the \a data in the structure under the \a key, which can be any kind 
 * of data, as long as its length is provided in \a key_len.
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
 * \brief Add data to the dict (null-terminated key)
 *
 * \param dict the head of the dict
 * \param key the key to set the new data
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a key as argument) when 
 *        \a key is removed from the dictionnary
 *
 * set the \a data in the structure under the \a key, which is a 
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
 * \brief Retrieve data from the dict (arbitrary key)
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \param key_len the size of the \a key
 * \returns the data that we are looking for
 *
 * Search the given \a key. throws not_found_error when not found.
 */
void *
xbt_dict_get_ext(xbt_dict_t      dict,
                 const char     *key,
                 int             key_len) {

  xbt_assert(dict);

  return xbt_dictelm_get_ext(dict->head, key, key_len);
}

/**
 * \brief Retrieve data from the dict (null-terminated key) 
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \returns the data that we are looking for
 *
 * Search the given \a key. THROWs mismatch_error when not found. 
 * Check xbt_dict_get_or_null() for a version returning NULL without exception when 
 * not found.
 */
void *
xbt_dict_get(xbt_dict_t     dict,
             const char     *key) {
  xbt_assert(dict);

  return xbt_dictelm_get(dict->head, key);
}

/**
 * \brief like xbt_dict_get(), but returning NULL when not found
 */
void *
xbt_dict_get_or_null(xbt_dict_t     dict,
		     const char     *key) {
  xbt_ex_t e;
  void *res;
  TRY {
    res = xbt_dictelm_get(dict->head, key);
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(e);
    res=NULL;
  }
  return res;
}


/**
 * \brief Remove data from the dict (arbitrary key)
 *
 * \param dict the trash can
 * \param key the key of the data to be removed
 * \param key_len the size of the \a key
 * 
 *
 * Remove the entry associated with the given \a key (throws not_found)
 */
void
xbt_dict_remove_ext(xbt_dict_t  dict,
                     const char  *key,
                     int          key_len) {
  xbt_assert(dict);
  
  xbt_dictelm_remove_ext(dict->head, key, key_len);
}

/**
 * \brief Remove data from the dict (null-terminated key)
 *
 * \param dict the head of the dict
 * \param key the key of the data to be removed
 *
 * Remove the entry associated with the given \a key
 */
void
xbt_dict_remove(xbt_dict_t  dict,
		 const char  *key) {
  if (!dict)
    THROW1(arg_error,0,"Asked to remove key %s from NULL dict",key);

  xbt_dictelm_remove(dict->head, key);
}


/**
 * \brief Outputs the content of the structure (debuging purpose) 
 *
 * \param dict the exibitionist
 * \param output a function to dump each data in the tree
 *
 * Ouputs the content of the structure. (for debuging purpose). \a ouput is a
 * function to output the data. If NULL, data won't be displayed, just the tree structure.
 */

void
xbt_dict_dump(xbt_dict_t     dict,
               void_f_pvoid_t *output) {

  printf("Dict %p:\n", (void*)dict);
  xbt_dictelm_dump(dict ? dict->head: NULL, output);
}
