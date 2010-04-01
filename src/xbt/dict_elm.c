/* $Id$ */

/* dict - a generic dictionary, variation over the B-tree concept           */

/* Copyright (c) 2003-2009 The SimGrid team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"       /* prototypes of this module */

XBT_LOG_EXTERNAL_CATEGORY(xbt_dict);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict_elm, xbt_dict,
                                "Dictionaries internals");

XBT_LOG_NEW_SUBCATEGORY(xbt_dict_add, xbt_dict,
                        "Dictionaries internals: elements addition");
XBT_LOG_NEW_SUBCATEGORY(xbt_dict_search, xbt_dict,
                        "Dictionaries internals: searching");
XBT_LOG_NEW_SUBCATEGORY(xbt_dict_remove, xbt_dict,
                        "Dictionaries internals: elements removal");
XBT_LOG_NEW_SUBCATEGORY(xbt_dict_collapse, xbt_dict,
                        "Dictionaries internals: post-removal cleanup");

xbt_mallocator_t dict_elm_mallocator = NULL;

xbt_dictelm_t xbt_dictelm_new(const char *key,
                              int key_len,
                              unsigned int hash_code,
                              void *content, void_f_pvoid_t free_f)
{
  xbt_dictelm_t element = xbt_mallocator_get(dict_elm_mallocator);

  element->dictielem = 0; /* please free the key on free */
  element->key = xbt_new(char, key_len + 1);
  memcpy((void *)element->key, (void *)key, key_len);
  element->key[key_len] = '\0';

  element->key_len = key_len;
  element->hash_code = hash_code;

  element->content = content;
  element->free_f = free_f;
  element->next = NULL;

  return element;
}

xbt_dictelm_t xbt_dictielm_new(uintptr_t key, unsigned int hash_code, uintptr_t content) {
  xbt_dictelm_t element = xbt_mallocator_get(dict_elm_mallocator);

  element->key = (void*)key;

  element->dictielem = 1; /* please DONT free the key on free */
  element->key_len = sizeof(uintptr_t);
  element->hash_code = hash_code;

  element->content = (void*)content;
  element->free_f = NULL;
  element->next = NULL;

  return element;
}

void xbt_dictelm_free(xbt_dictelm_t element)
{
  if (element != NULL) {
    if (!element->dictielem)
      xbt_free(element->key);

    if (element->free_f != NULL && element->content != NULL) {
      element->free_f(element->content);
    }

    xbt_mallocator_release(dict_elm_mallocator, element);
  }
}

void *dict_elm_mallocator_new_f(void)
{
  return xbt_new(s_xbt_dictelm_t, 1);
}

void dict_elm_mallocator_free_f(void *elem)
{
  xbt_free(elem);
}

void dict_elm_mallocator_reset_f(void *elem)
{

}
