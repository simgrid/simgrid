/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Copyright (c) 2004-2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DICT_PRIVATE_H__
#define _XBT_DICT_PRIVATE_H__

#include "xbt/base.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"

#define MAX_FILL_PERCENT 80

typedef struct s_xbt_het_dictelm {
  s_xbt_dictelm_t element;
  void_f_pvoid_t free_f;
} s_xbt_het_dictelm_t, *xbt_het_dictelm_t;

typedef struct s_xbt_dict {
  void_f_pvoid_t free_f;
  xbt_dictelm_t *table;
  int table_size;
  int count;
  int fill;
  int homogeneous;
} s_xbt_dict_t;

typedef struct s_xbt_dict_cursor s_xbt_dict_cursor_t;

extern XBT_PRIVATE xbt_mallocator_t dict_elm_mallocator;
XBT_PRIVATE void * dict_elm_mallocator_new_f(void);
#define dict_elm_mallocator_free_f xbt_free_f
#define dict_elm_mallocator_reset_f ((void_f_pvoid_t)NULL)

extern XBT_PRIVATE xbt_mallocator_t dict_het_elm_mallocator;
extern XBT_PRIVATE void * dict_het_elm_mallocator_new_f(void);
#define dict_het_elm_mallocator_free_f xbt_free_f
#define dict_het_elm_mallocator_reset_f ((void_f_pvoid_t)NULL)

/*####[ Function prototypes ]################################################*/
XBT_PRIVATE xbt_dictelm_t xbt_dictelm_new(xbt_dict_t dict, const char *key, int key_len,
                              unsigned int hash_code, void *content, void_f_pvoid_t free_f);
XBT_PRIVATE void xbt_dictelm_free(xbt_dict_t dict, xbt_dictelm_t element);
XBT_PRIVATE void xbt_dictelm_set_data(xbt_dict_t dict, xbt_dictelm_t element, void *data, void_f_pvoid_t free_ctn);

#endif                          /* _XBT_DICT_PRIVATE_H_ */
