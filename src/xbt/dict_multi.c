/* $Id$ */

/* dict_multi - dictionnaries of dictionnaries of ... of data               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 Martin Quinson.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen() */

/*####[ Multi dict functions ]##############################################*/
/*###############################"##########################################*/
/**
 * gras_mutidict_set:
 *
 * @head: the head of dict
 * @keycount: the number of the key
 * @key: the key
 * @data: the data to set
 * @Returns: gras_error_t
 *
 * set the data in the structure under the @keycount @key.
 */

gras_error_t
gras_multidict_set_ext(gras_dict_t    **pp_head,
                          int              keycount,
                          char           **key,
                          int             *key_len,
                          void            *data,
                          void_f_pvoid_t  *free_ctn) {
  gras_error_t  errcode   = no_error;
  gras_dictelm_t  *p_elm    =     NULL;
  gras_dictelm_t  *p_subdict =     NULL;
  int           i         =        0;

  CDEBUG2(dict_multi, "fast_multidict_set(%p,%d). Keys:", *pp_head, keycount);

  /*
  for (i = 0; i < keycount; i++) {
    CDEBUG1(dict_multi, "\"%s\"", key[i]);
  }
  */

  gras_assert0(keycount >= 1, "Can't set less than one key in a multidict");

  if (keycount == 1)
    return gras_dict_set_ext(pp_head, key[0], key_len[0], data, free_ctn);

  if (!*pp_head) {
    TRY(_gras_dict_alloc(NULL, 0, 0, NULL, NULL, pp_head));
  }

  p_elm = *pp_head;

  for (i = 0; i < keycount-1; i++) {

    /* search the dict of next level */
    TRYCATCH(gras_dict_get(p_elm, key[i], (void*)&p_subdict), mismatch_error);

    /* make sure the dict of next level exists */
    if (errcode == mismatch_error) {
      TRY(_gras_dict_alloc(NULL, 0, 0, NULL, NULL, &p_subdict));
      TRY(gras_dict_set_ext(&p_elm, key[i], key_len[i], &p_subdict,
                               _free_dict));
    }

    p_elm = p_subdict;
  }

  return gras_dict_set_ext(&p_elm, key[i], key_len[i], data, free_ctn);
}

gras_error_t
gras_multidict_set(gras_dictelm_t    **pp_head,
                      int              keycount,
                      char           **key,
                      void            *data,
                      void_f_pvoid_t  *free_ctn) {
  gras_error_t  errcode = no_error;
  int          *key_len = NULL;
  int           i       = 0;

  key_len = malloc(keycount * sizeof (int));
  if (!key_len)
    RAISE_MALLOC;

  for (i = 0; i < keycount; i++) {
    key_len[i] = 1+strlen(key[i]);
  }

  TRYCLEAN(gras_multidict_set_ext(pp_head, keycount, key, key_len, data, free_ctn),
           free(key_len));

  free(key_len);

  return errcode;
}

/**
 * gras_mutidict_get:
 *
 * @head: the head of dict
 * @keycount: the number of the key
 * @key: the key
 * @data: where to put the got data
 * @Returns: gras_error_t
 *
 * Search the given @key. data=NULL when not found
 */


gras_error_t
gras_multidict_get_ext(gras_dictelm_t    *p_head,
                            int             keycount,
                            const char    **key,
                            int            *key_len,
                            /* OUT */void **data) {
  gras_error_t  errcode = no_error;
  gras_dictelm_t  *p_elm  =   p_head;
  int           i       =        0;

  CDEBUG2(dict_multi, "fast_multidict_get(%p, %d). Keys:", p_head, keycount);

  /*
  for (i = 0; i < keycount; i++) {
    CDEBUG1(dict_multi, "\"%s\"", key[i]);
  }
  */

  i = 0;

  while (p_elm && i < keycount-1) {

    TRY(gras_dict_get_ext(p_elm, key[i], key_len[i], (void**)p_elm));

    /*
    if (p_elm) {
      CDEBUG3(dict_multi,"Found level %d for key %s in multitree %", i, key[i], p_head);
    } else {
      CDEBUG3(dict_multi,"NOT found level %d for key %s in multitree %p", i, key[i], p_head);
    }
    */

    i++;
  }

  if (p_elm) { /* Found all dicts to the data */

    /*    gras_dict_dump(dict,&gras_dict_prints); */
    return gras_dict_get_ext(p_elm, key[i], key_len[i], data);

  } else {

    *data = NULL;

    return 1;
  }

}

gras_error_t
gras_multidict_get(gras_dictelm_t    *p_head,
                        int             keycount,
                        const char    **key,
                        /* OUT */void **data) {
  gras_error_t  errcode = no_error;
  int          *key_len = NULL;
  int           i       = 0;

  key_len = malloc(keycount * sizeof (int));
  if (!key_len)
    RAISE_MALLOC;

  for (i = 0; i < keycount; i++) {
    key_len[i] = 1+strlen(key[i]);
  }

  TRYCLEAN(gras_multidict_get_ext(p_head, keycount, key, key_len, data),
           free(key_len));
  free(key_len);

  return errcode;
}


/**
 *  gras_mutidict_remove:
 *
 *  @head: the head of dict
 *  @keycount: the number of the key
 *  @key: the key
 *  @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 * Removing a non-existant key is ok.
 */

gras_error_t
gras_multidict_remove_ext(gras_dictelm_t  *p_head,
                          int           keycount,
                          const char  **key,
                          int          *key_len) {
  gras_dictelm_t *p_elm = p_head;
  int          i      =      0;

  while (p_elm && i < keycount-1) {
    if (!gras_dict_get_ext(p_elm, key[i], key_len[i], (void**)&p_elm)) {
      return 0;
    }
  }

  if (p_elm) {
    /* Found all dicts to the data */
    return gras_dict_remove_ext(p_elm, key[i], key_len[i]);
  } else {
    return 1;
  }

}

gras_error_t
gras_multidict_remove(gras_dictelm_t  *p_head,
                      int           keycount,
                      const char  **key) {
  gras_error_t  errcode = no_error;
  int          *key_len = NULL;
  int           i       = 0;

  key_len = malloc(keycount * sizeof (int));
  if (!key_len)
    RAISE_MALLOC;

  for (i = 0; i < keycount; i++) {
    key_len[i] = 1+strlen(key[i]);
  }

  TRYCLEAN(gras_multidict_remove_ext(p_head, keycount, key, key_len),
           free(key_len));
  free(key_len);

  return errcode;
}
