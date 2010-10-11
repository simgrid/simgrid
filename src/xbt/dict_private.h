/* dict_elm - elements of generic dictionnaries                             */
/* This file is not to be loaded from anywhere but dict.c                   */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_DICT_PRIVATE_H__
#define _XBT_DICT_PRIVATE_H__

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"

typedef struct xbt_dictelm_ *xbt_dictelm_t;

#define MAX_FILL_PERCENT 80

typedef struct xbt_dictelm_ {
  int dictielem:1;
  char *key;
  int key_len;
  unsigned int hash_code;

  void *content;
  void_f_pvoid_t free_f;

  xbt_dictelm_t next;
} s_xbt_dictelm_t;

typedef struct xbt_dict_ {
  xbt_dictelm_t *table;
  int table_size;
  int count;
  int fill;
} s_xbt_dict_t;

typedef struct xbt_dict_cursor_ s_xbt_dict_cursor_t;

extern xbt_mallocator_t dict_elm_mallocator;
extern void *dict_elm_mallocator_new_f(void);
extern void dict_elm_mallocator_free_f(void *elem);
extern void dict_elm_mallocator_reset_f(void *elem);

/*####[ Function prototypes ]################################################*/
xbt_dictelm_t xbt_dictelm_new(const char *key, int key_len,
                              unsigned int hash_code, void *content,
                              void_f_pvoid_t free_f);
xbt_dictelm_t xbt_dictielm_new(uintptr_t key, unsigned int hash_code,
                               uintptr_t content);
void xbt_dictelm_free(xbt_dictelm_t element);
void xbt_dict_add_element(xbt_dict_t dict, xbt_dictelm_t element);

#endif                          /* _XBT_DICT_PRIVATE_H_ */
