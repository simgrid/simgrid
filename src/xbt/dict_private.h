/* $Id$ */

/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 Martin Quinson.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _GRAS_DICT_ELM_T_
#define _GRAS_DICT_ELM_T_

/*####[ Type definition ]####################################################*/
typedef struct gras_dictelm_ {
  char           *key;
  int             key_len;
  int             offset; /* offset on the key */
  void           *content;
  void_f_pvoid_t *free_ctn; /*pointer to the function to call to free this ctn*/

  gras_dynar_t   *sub; /* sub */
} gras_dictelm_t;

struct gras_dict_ {
  gras_dictelm_t *head;
};

/*####[ Function prototypes ]################################################*/
void         gras_dictelm_free        (gras_dictelm_t **pp_elm);

gras_error_t gras_dictelm_insert      (gras_dictelm_t **pp_head,
				       const char      *_key,
				       void            *data,
				       void_f_pvoid_t  *free_ctn);
gras_error_t gras_dictelm_insert_ext  (gras_dictelm_t **pp_head,
				       const char      *_key,
				       int              key_len,
				       void            *data,
				       void_f_pvoid_t  *free_ctn);

gras_error_t gras_dictelm_retrieve    (gras_dictelm_t *p_head,
				       const char     *key,
				       /* OUT */void **data);
gras_error_t gras_dictelm_retrieve_ext(gras_dictelm_t *p_head,
				       const char     *key,
				       int             key_len,
				       /* OUT */void **data);

gras_error_t gras_dictelm_remove      (gras_dictelm_t *p_head,
				       const char  *key);
gras_error_t gras_dictelm_remove_ext  (gras_dictelm_t *p_head,
				       const char  *key,
				       int          key_len);

gras_error_t gras_dictelm_dump        (gras_dictelm_t *p_head,
				       void_f_pvoid_t *output);

void         gras_dictelm_print_fct   (void *data);

#endif  /* _GRAS_DICT_ELM_T_ */

