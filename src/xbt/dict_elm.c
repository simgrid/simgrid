/* dict - a generic dictionary, variation over hash table                   */

/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"       /* prototypes of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict_elm, xbt_dict, "Dictionaries internals");

xbt_mallocator_t dict_elm_mallocator = NULL;
xbt_mallocator_t dict_het_elm_mallocator = NULL;

xbt_dictelm_t xbt_dictelm_new(xbt_dict_t dict, const char *key, int key_len, unsigned int hash_code, void *content,
                              void_f_pvoid_t free_f)
{
  xbt_dictelm_t element;

  if (dict->homogeneous) {
    xbt_assert(!free_f, "Cannot set an individual free function in homogeneous dicts.");
    element = xbt_mallocator_get(dict_elm_mallocator);
  } else {
    xbt_het_dictelm_t het_element = xbt_mallocator_get(dict_het_elm_mallocator);
    het_element->free_f = free_f;
    element = &het_element->element;
  }
  element->key = xbt_new(char, key_len + 1);
  memcpy(element->key, key, key_len);
  element->key[key_len] = '\0';

  element->key_len = key_len;
  element->hash_code = hash_code;

  element->content = content;
  element->next = NULL;

  return element;
}

void xbt_dictelm_free(xbt_dict_t dict, xbt_dictelm_t element)
{
  if (element) {
    char *key = element->key;
    void *content = element->content;
    void_f_pvoid_t free_f;
    if (dict->homogeneous) {
      free_f = dict->free_f;
      xbt_mallocator_release(dict_elm_mallocator, element);
    } else {
      xbt_het_dictelm_t het_element = (xbt_het_dictelm_t)element;
      free_f = het_element->free_f;
      xbt_mallocator_release(dict_het_elm_mallocator, het_element);
    }

    xbt_free(key);
    if (free_f && content)
      free_f(content);
  }
}

void xbt_dictelm_set_data(xbt_dict_t dict, xbt_dictelm_t element, void *data, void_f_pvoid_t free_ctn)
{
  void_f_pvoid_t free_f;
  if (dict->homogeneous) {
    free_f = dict->free_f;
    xbt_assert(!free_ctn, "Cannot set an individual free function in homogeneous dicts.");
  } else {
    xbt_het_dictelm_t het_element = (xbt_het_dictelm_t)element;
    free_f  = het_element->free_f;
    het_element->free_f = free_ctn;
  }

  if (free_f && element->content)
    free_f(element->content);

  element->content = data;
}

void *dict_elm_mallocator_new_f(void)
{
  return xbt_new(s_xbt_dictelm_t, 1);
}

void *dict_het_elm_mallocator_new_f(void)
{
  return xbt_new(s_xbt_het_dictelm_t, 1);
}
