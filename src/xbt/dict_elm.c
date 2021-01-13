/* dict - a generic dictionary, variation over hash table                   */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"       /* prototypes of this module */

xbt_mallocator_t dict_elm_mallocator = NULL;

xbt_dictelm_t xbt_dictelm_new(const char* key, int key_len, unsigned int hash_code, void* content)
{
  xbt_dictelm_t element = xbt_mallocator_get(dict_elm_mallocator);
  element->key = xbt_new(char, key_len + 1);
  memcpy(element->key, key, key_len);
  element->key[key_len] = '\0';

  element->key_len = key_len;
  element->hash_code = hash_code;

  element->content = content;
  element->next = NULL;

  return element;
}

void xbt_dictelm_free(const_xbt_dict_t dict, xbt_dictelm_t element)
{
  if (element) {
    char *key = element->key;
    void *content = element->content;
    void_f_pvoid_t free_f;
    free_f = dict->free_f;
    xbt_mallocator_release(dict_elm_mallocator, element);

    xbt_free(key);
    if (free_f && content)
      free_f(content);
  }
}

void xbt_dictelm_set_data(const_xbt_dict_t dict, xbt_dictelm_t element, void* data)
{
  void_f_pvoid_t free_f = dict->free_f;
  if (free_f && element->content)
    free_f(element->content);

  element->content = data;
}

void *dict_elm_mallocator_new_f(void)
{
  return xbt_new(s_xbt_dictelm_t, 1);
}
