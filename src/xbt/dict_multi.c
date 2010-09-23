/* dict_multi - dictionnaries of dictionnaries of ... of data               */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict_multi, xbt_dict,
                                "Dictionaries of multiple keys");

static void _free_dict(void *d)
{
  VERB1("free dict %p", d);
  xbt_dict_free((xbt_dict_t *) & d);
}

/** \brief Insert \e data under all the keys contained in \e keys, providing their sizes in \e lens.
 *
 * \arg mdict: the multi-dict
 * \arg keys: dynar of (char *) containing all the keys
 * \arg lens: length of each element of \e keys
 * \arg data: what to store in the structure
 * \arg free_ctn: function to use to free the pushed content on need
 *
 * Dynars are not modified during the operation.
 */

void
xbt_multidict_set_ext(xbt_dict_t mdict,
                      xbt_dynar_t keys, xbt_dynar_t lens,
                      void *data, void_f_pvoid_t free_ctn)
{

  xbt_dict_t thislevel, nextlevel = NULL;
  int i;

  unsigned long int thislen;
  char *thiskey;
  int keys_len = xbt_dynar_length(keys);

  xbt_assert(xbt_dynar_length(keys) == xbt_dynar_length(lens));
  xbt_assert0(keys_len, "Can't set a zero-long key set in a multidict");

  DEBUG2("xbt_multidict_set(%p,%d)", mdict, keys_len);

  for (i = 0, thislevel = mdict; i < keys_len - 1; i++, thislevel = nextlevel) {

    xbt_dynar_get_cpy(keys, i, &thiskey);
    xbt_dynar_get_cpy(lens, i, &thislen);

    DEBUG5("multi_set: at level %d, len=%ld, key=%p |%*s|", i, thislen,
           thiskey, (int) thislen, thiskey);

    /* search the dict of next level */
    nextlevel = xbt_dict_get_or_null_ext(thislevel, thiskey, thislen);
    if (nextlevel == NULL) {
      /* make sure the dict of next level exists */
      nextlevel = xbt_dict_new();
      VERB1("Create a dict (%p)", nextlevel);
      xbt_dict_set_ext(thislevel, thiskey, thislen, nextlevel, &_free_dict);
    }
  }

  xbt_dynar_get_cpy(keys, i, &thiskey);
  xbt_dynar_get_cpy(lens, i, &thislen);

  xbt_dict_set_ext(thislevel, thiskey, thislen, data, free_ctn);
}

/** \brief Insert \e data under all the keys contained in \e keys
 *
 * \arg head: the head of dict
 * \arg keys: dynar of null-terminated strings containing all the keys
 * \arg data: what to store in the structure
 * \arg free_ctn: function to use to free the pushed content on need
 */
void
xbt_multidict_set(xbt_dict_t mdict,
                  xbt_dynar_t keys, void *data, void_f_pvoid_t free_ctn)
{
  xbt_dynar_t lens = xbt_dynar_new(sizeof(unsigned long int), NULL);
  unsigned long i;
  xbt_ex_t e;

  for (i = 0; i < xbt_dynar_length(keys); i++) {
    char *thiskey = xbt_dynar_get_as(keys, i, char *);
    unsigned long int thislen = (unsigned long int) strlen(thiskey);
    DEBUG2("Push %ld as level %lu length", thislen, i);
    xbt_dynar_push(lens, &thislen);
  }

  TRY {
    xbt_multidict_set_ext(mdict, keys, lens, data, free_ctn);
  } _CLEANUP {
    xbt_dynar_free(&lens);
  } CATCH(e) {
    RETHROW;
  }
}

/** \brief Insert \e data under all the keys contained in \e keys, providing their sizes in \e lens.
 *
 * \arg mdict: the multi-dict
 * \arg keys: dynar of (char *) containing all the keys
 * \arg lens: length of each element of \e keys
 * \arg data: where to put what was found in structure
 * \arg free_ctn: function to use to free the pushed content on need
 *
 * Dynars are not modified during the operation.
 */
void *xbt_multidict_get_ext(xbt_dict_t mdict,
                            xbt_dynar_t keys, xbt_dynar_t lens)
{
  xbt_dict_t thislevel, nextlevel;
  int i;

  unsigned long int thislen;
  char *thiskey;
  int keys_len = xbt_dynar_length(keys);

  xbt_assert(xbt_dynar_length(keys) == xbt_dynar_length(lens));
  xbt_assert0(xbt_dynar_length(keys) >= 1,
              "Can't get a zero-long key set in a multidict");

  DEBUG2("xbt_multidict_get(%p, %ld)", mdict, xbt_dynar_length(keys));

  for (i = 0, thislevel = mdict; i < keys_len - 1; i++, thislevel = nextlevel) {

    xbt_dynar_get_cpy(keys, i, &thiskey);
    xbt_dynar_get_cpy(lens, i, &thislen);

    DEBUG6("multi_get: at level %d (%p), len=%ld, key=%p |%*s|",
           i, thislevel, thislen, thiskey, (int) thislen, thiskey);

    /* search the dict of next level: let mismatch raise if not found */
    nextlevel = xbt_dict_get_ext(thislevel, thiskey, thislen);
  }

  xbt_dynar_get_cpy(keys, i, &thiskey);
  xbt_dynar_get_cpy(lens, i, &thislen);

  return xbt_dict_get_ext(thislevel, thiskey, thislen);
}

void *xbt_multidict_get(xbt_dict_t mdict, xbt_dynar_t keys)
{
  xbt_dynar_t lens = xbt_dynar_new(sizeof(unsigned long int), NULL);
  unsigned long i;
  void *res;

  for (i = 0; i < xbt_dynar_length(keys); i++) {
    char *thiskey = xbt_dynar_get_as(keys, i, char *);
    unsigned long int thislen = (unsigned long int) strlen(thiskey);
    xbt_dynar_push(lens, &thislen);
  }

  res = xbt_multidict_get_ext(mdict, keys, lens), xbt_dynar_free(&lens);
  return res;
}


/** \brief Remove the entry under all the keys contained in \e keys, providing their sizes in \e lens.
 *
 * \arg mdict: the multi-dict
 * \arg keys: dynar of (char *) containing all the keys
 * \arg lens: length of each element of \e keys
 * \arg data: what to store in the structure
 * \arg free_ctn: function to use to free the pushed content on need
 *
 * Dynars are not modified during the operation.
 *
 * Removing a non-existant key is ok.
 */

void
xbt_multidict_remove_ext(xbt_dict_t mdict, xbt_dynar_t keys, xbt_dynar_t lens)
{
  xbt_dict_t thislevel, nextlevel = NULL;
  int i;
  xbt_ex_t e;

  unsigned long int thislen;
  char *thiskey;
  int keys_len = xbt_dynar_length(keys);

  xbt_assert(xbt_dynar_length(keys) == xbt_dynar_length(lens));
  xbt_assert0(xbt_dynar_length(keys),
              "Can't remove a zero-long key set in a multidict");

  for (i = 0, thislevel = mdict; i < keys_len - 1; i++, thislevel = nextlevel) {

    xbt_dynar_get_cpy(keys, i, &thiskey);
    xbt_dynar_get_cpy(lens, i, &thislen);

    /* search the dict of next level */
    TRY {
      nextlevel = xbt_dict_get_ext(thislevel, thiskey, thislen);
    }
    CATCH(e) {
      /* If non-existant entry, nothing to do */
      if (e.category == arg_error)
        xbt_ex_free(e);
      else
        RETHROW;
    }
  }

  xbt_dynar_get_cpy(keys, i, &thiskey);
  xbt_dynar_get_cpy(lens, i, &thislen);

  xbt_dict_remove_ext(thislevel, thiskey, thislen);
}

void xbt_multidict_remove(xbt_dict_t mdict, xbt_dynar_t keys)
{

  xbt_ex_t e;
  xbt_dynar_t lens = xbt_dynar_new(sizeof(unsigned long int), NULL);
  unsigned long i;

  for (i = 0; i < xbt_dynar_length(keys); i++) {
    char *thiskey = xbt_dynar_get_as(keys, i, char *);
    unsigned long int thislen = strlen(thiskey);
    xbt_dynar_push(lens, &thislen);
  }

  TRY {
    xbt_multidict_remove_ext(mdict, keys, lens);
  } _CLEANUP {
    xbt_dynar_free(&lens);
  } CATCH(e) {
    RETHROW;
  }
}
