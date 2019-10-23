/* dict_elm - elements of generic dictionaries                             */
/* This file is not to be loaded from anywhere but dict.cpp                 */

/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_DICT_PRIVATE_H
#define XBT_DICT_PRIVATE_H

#include "xbt/base.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"

SG_BEGIN_DECL()

typedef struct s_xbt_dict {
  void_f_pvoid_t free_f;
  xbt_dictelm_t *table;
  int table_size;
  int count;
  int fill;
} s_xbt_dict_t;

extern XBT_PRIVATE xbt_mallocator_t dict_elm_mallocator;
XBT_PRIVATE void * dict_elm_mallocator_new_f(void);
#define dict_elm_mallocator_free_f xbt_free_f
#define dict_elm_mallocator_reset_f ((void_f_pvoid_t)NULL)

/*####[ Function prototypes ]################################################*/
XBT_PRIVATE xbt_dictelm_t xbt_dictelm_new(const char* key, int key_len, unsigned int hash_code, void* content);
XBT_PRIVATE void xbt_dictelm_free(xbt_dict_t dict, xbt_dictelm_t element);
XBT_PRIVATE void xbt_dictelm_set_data(xbt_dict_t dict, xbt_dictelm_t element, void* data);

SG_END_DECL()

#endif
