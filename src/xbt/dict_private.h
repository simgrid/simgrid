/* $Id$ */

/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DICT_ELM_T_
#define _XBT_DICT_ELM_T_

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

/*####[ Type definition ]####################################################*/
typedef struct xbt_dictelm_ {
  char           *key;
  int             key_len;
  int             offset; /* offset on the key */
  int             internal; /* true if it's only an internal node */
  void           *content;
  void_f_pvoid_t *free_f; /*pointer to the function to call to free this ctn*/

  xbt_dynar_t    sub; /* sub */
} s_xbt_dictelm_t, *xbt_dictelm_t;

typedef struct xbt_dict_ {
  s_xbt_dictelm_t *head;
} s_xbt_dict_t;

typedef struct xbt_dict_cursor_ s_xbt_dict_cursor_t;

/*####[ Function prototypes ]################################################*/
void  xbt_dictelm_free      (s_xbt_dictelm_t **pp_elm);

void  xbt_dictelm_set       (s_xbt_dictelm_t **pp_head,
			     const char      *_key,
			     void            *data,
			     void_f_pvoid_t  *free_ctn);
void  xbt_dictelm_set_ext   (s_xbt_dictelm_t **pp_head,
			     const char      *_key,
			     int              key_len,
			     void            *data,
			     void_f_pvoid_t  *free_ctn);

void* xbt_dictelm_get       (s_xbt_dictelm_t *p_head, 
	                     const char     *key);
void* xbt_dictelm_get_ext   (s_xbt_dictelm_t *p_head,
			     const char     *key,
			     int             key_len);

void xbt_dictelm_remove    (s_xbt_dictelm_t *p_head,
			    const char  *key);
void xbt_dictelm_remove_ext(s_xbt_dictelm_t *p_head,
			    const char  *key,
			    int          key_len);

void         xbt_dictelm_dump      (s_xbt_dictelm_t *p_head,
				     void_f_pvoid_t *output);

void         xbt_dictelm_print_fct (void *data);

#endif  /* _XBT_DICT_ELM_T_ */

