/* $Id$ */

/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 Martin Quinson.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _GRAS_DICT_ELM_T_
#define _GRAS_DICT_ELM_T_

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

/*####[ Type definition ]####################################################*/
typedef struct gras_dictelm_ {
  char           *key;
  int             key_len;
  int             offset; /* offset on the key */
  void           *content;
  void_f_pvoid_t *free_f; /*pointer to the function to call to free this ctn*/

  gras_dynar_t    sub; /* sub */
} s_gras_dictelm_t, *gras_dictelm_t;

typedef struct gras_dict_ {
  s_gras_dictelm_t *head;
} s_gras_dict_t;

typedef struct gras_dict_cursor_ s_gras_dict_cursor_t;

/*####[ Function prototypes ]################################################*/
void gras_dictelm_free      (s_gras_dictelm_t **pp_elm);

void gras_dictelm_set       (s_gras_dictelm_t **pp_head,
			     const char      *_key,
			     void            *data,
			     void_f_pvoid_t  *free_ctn);
void gras_dictelm_set_ext   (s_gras_dictelm_t **pp_head,
			     const char      *_key,
			     int              key_len,
			     void            *data,
			     void_f_pvoid_t  *free_ctn);

gras_error_t gras_dictelm_get       (s_gras_dictelm_t *p_head,
				     const char     *key,
				     /* OUT */void **data);
gras_error_t gras_dictelm_get_ext   (s_gras_dictelm_t *p_head,
				     const char     *key,
				     int             key_len,
				     /* OUT */void **data);

gras_error_t gras_dictelm_remove    (s_gras_dictelm_t *p_head,
				     const char  *key);
gras_error_t gras_dictelm_remove_ext(s_gras_dictelm_t *p_head,
				       const char  *key,
				       int          key_len);

void         gras_dictelm_dump      (s_gras_dictelm_t *p_head,
				     void_f_pvoid_t *output);

void         gras_dictelm_print_fct (void *data);

#endif  /* _GRAS_DICT_ELM_T_ */

