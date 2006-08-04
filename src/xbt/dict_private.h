/* $Id$ */

/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DICT_PRIVATE_H__
#define _XBT_DICT_PRIVATE_H__

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

typedef struct xbt_dictelm_ *xbt_dictelm_t;

typedef struct xbt_dictelm_ {
  char *key;
  int key_len;
  void *content;
  void_f_pvoid_t *free_f;
  xbt_dictelm_t next;
} s_xbt_dictelm_t;

typedef struct xbt_dict_ {
  xbt_dictelm_t *table;
  int table_size;
  int count;
} s_xbt_dict_t;

typedef struct xbt_dict_cursor_ s_xbt_dict_cursor_t;

unsigned int xbt_dict_hash(const char *str);

/*####[ Function prototypes ]################################################*/
xbt_dictelm_t xbt_dictelm_new(const char *key,
			      int key_len,
			      void *content,
			      void_f_pvoid_t free_f,
			      xbt_dictelm_t next);
void xbt_dictelm_free(xbt_dictelm_t element);
void xbt_dict_add_element(xbt_dict_t dict, xbt_dictelm_t element);

#endif  /* _XBT_DICT_PRIVATE_H_ */
