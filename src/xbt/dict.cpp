/* dict - a generic dictionary, variation over hash table                   */

/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "dict_private.h"
#include "simgrid/Exception.hpp"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "xbt/str.h"
#include "xbt/string.hpp"
#include "xbt/xbt_modinter.h"

#include <cstdio>
#include <cstring>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict, xbt, "Dictionaries provide the same functionalities as hash tables");

constexpr int MAX_FILL_PERCENT = 80;

/**
 * @brief Constructor
 * @param free_ctn function to call with (@a data as argument) when @a data is removed from the dictionary
 * @return pointer to the destination
 * @see xbt_dict_free()
 *
 * Creates and initialize a new dictionary with a default hashtable size.
 * The dictionary is homogeneous: each element share the same free function.
 */
xbt_dict_t xbt_dict_new_homogeneous(void_f_pvoid_t free_ctn)
{
  xbt_dict_preinit();

  xbt_dict_t dict;

  dict = xbt_new(s_xbt_dict_t, 1);
  dict->free_f = free_ctn;
  dict->table_size = 127;
  dict->table = xbt_new0(xbt_dictelm_t, dict->table_size + 1);
  dict->count = 0;
  dict->fill = 0;

  return dict;
}

/**
 * @brief Destructor
 * @param dict the dictionary to be freed
 *
 * Frees a dictionary with all the data
 */
void xbt_dict_free(xbt_dict_t * dict)
{
  if (dict != nullptr && *dict != nullptr) {
    int table_size       = (*dict)->table_size;
    xbt_dictelm_t* table = (*dict)->table;
    /* Warning: the size of the table is 'table_size+1'...
     * This is because table_size is used as a binary mask in xbt_dict_rehash */
    for (int i = 0; (*dict)->count && i <= table_size; i++) {
      xbt_dictelm_t current = table[i];
      xbt_dictelm_t previous;

      while (current != nullptr) {
        previous = current;
        current = current->next;
        xbt_dictelm_free(*dict, previous);
        (*dict)->count--;
      }
    }
    xbt_free(table);
    xbt_free(*dict);
    *dict = nullptr;
  }
}

/** Returns the amount of elements in the dict */
unsigned int xbt_dict_size(const_xbt_dict_t dict)
{
  return (dict != nullptr ? static_cast<unsigned int>(dict->count) : static_cast<unsigned int>(0));
}

/* Expend the size of the dict */
static void xbt_dict_rehash(xbt_dict_t dict)
{
  const unsigned oldsize = dict->table_size + 1;
  unsigned newsize = oldsize * 2;

  auto* newtable = static_cast<xbt_dictelm_t*>(xbt_realloc((char*)dict->table, newsize * sizeof(xbt_dictelm_t)));
  memset(&newtable[oldsize], 0, oldsize * sizeof(xbt_dictelm_t)); /* zero second half */
  newsize--;
  dict->table_size = newsize;
  dict->table      = newtable;
  XBT_DEBUG("REHASH (%u->%u)", oldsize, newsize);

  for (unsigned i = 0; i < oldsize; i++) {
    xbt_dictelm_t* currcell = &newtable[i];
    if (*currcell == nullptr) /* empty cell */
      continue;

    xbt_dictelm_t *twincell = currcell + oldsize;
    xbt_dictelm_t *pprev = currcell;
    xbt_dictelm_t bucklet = *currcell;
    while (bucklet != nullptr) {
      /* Since we use "& size" instead of "%size" and since the size was doubled, each bucklet of this cell must either:
         - stay  in  cell i (ie, currcell)
         - go to the cell i+oldsize (ie, twincell) */
      if ((bucklet->hash_code & newsize) != i) {        /* Move to b */
        *pprev = bucklet->next;
        bucklet->next = *twincell;
        if (*twincell == nullptr)
          dict->fill++;
        *twincell = bucklet;
      } else {
        pprev = &bucklet->next;
      }
      bucklet = *pprev;
    }

    if (*currcell == nullptr) /* everything moved */
      dict->fill--;
  }
}

/**
 * @brief Add data to the dict (arbitrary key)
 * @param dict the container
 * @param key the key to set the new data
 * @param key_len the size of the @a key
 * @param data the data to add in the dict
 *
 * Set the @a data in the structure under the @a key, which can be any kind of data, as long as its length is provided
 * in @a key_len.
 */
void xbt_dict_set_ext(xbt_dict_t dict, const char* key, int key_len, void* data)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);

  xbt_dictelm_t current;
  xbt_dictelm_t previous = nullptr;

  XBT_CDEBUG(xbt_dict, "ADD %.*s hash = %u, size = %d, & = %u", key_len, key, hash_code,
             dict->table_size, hash_code & dict->table_size);
  current = dict->table[hash_code & dict->table_size];
  while (current != nullptr && (hash_code != current->hash_code || key_len != current->key_len
          || memcmp(key, current->key, key_len))) {
    previous = current;
    current = current->next;
  }

  if (current == nullptr) {
    /* this key doesn't exist yet */
    current = xbt_dictelm_new(key, key_len, hash_code, data);
    dict->count++;
    if (previous == nullptr) {
      dict->table[hash_code & dict->table_size] = current;
      dict->fill++;
      if ((dict->fill * 100) / (dict->table_size + 1) > MAX_FILL_PERCENT)
        xbt_dict_rehash(dict);
    } else {
      previous->next = current;
    }
  } else {
    XBT_CDEBUG(xbt_dict, "Replace %.*s by %.*s under key %.*s",
               key_len, (char *) current->content, key_len, (char *) data, key_len, (char *) key);
    /* there is already an element with the same key: overwrite it */
    xbt_dictelm_set_data(dict, current, data);
  }
}

/**
 * @brief Add data to the dict (null-terminated key)
 *
 * @param dict the dict
 * @param key the key to set the new data
 * @param data the data to add in the dict
 *
 * set the @a data in the structure under the @a key, which is a null terminated string.
 */
void xbt_dict_set(xbt_dict_t dict, const char* key, void* data)
{
  xbt_dict_set_ext(dict, key, strlen(key), data);
}

/**
 * @brief Retrieve data from the dict (arbitrary key)
 *
 * @param dict the dealer of data
 * @param key the key to find data
 * @param key_len the size of the @a key
 * @return the data that we are looking for
 *
 * Search the given @a key. Returns nullptr when not found.
 */
void* xbt_dict_get_or_null_ext(const_xbt_dict_t dict, const char* key, int key_len)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);
  const s_xbt_dictelm* current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || key_len != current->key_len
          || memcmp(key, current->key, key_len))) {
    current = current->next;
  }

  if (current == nullptr)
    return nullptr;

  return current->content;
}

/**
 * @brief Retrieve data from the dict (null-terminated key)
 *
 * @param dict the dealer of data
 * @param key the key to find data
 * @return the data that we are looking for
 *
 * Search the given @a key. Returns nullptr when not found.
 */
void* xbt_dict_get_or_null(const_xbt_dict_t dict, const char* key)
{
  const s_xbt_dictelm* current = xbt_dict_get_elm_or_null(dict, key);

  if (current == nullptr)
    return nullptr;

  return current->content;
}

/**
 * @brief Retrieve element from the dict (null-terminated key)
 *
 * @param dict the dealer of data
 * @param key the key to find data
 * @return the s_xbt_dictelm_t that we are looking for
 *
 * Search the given @a key. Returns nullptr when not found.
 */
xbt_dictelm_t xbt_dict_get_elm_or_null(const_xbt_dict_t dict, const char* key)
{
  unsigned int hash_code = xbt_str_hash(key);
  xbt_dictelm_t current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || strcmp(key, current->key)))
    current = current->next;
  return current;
}

/**
 * @brief Remove data from the dict (arbitrary key)
 *
 * @param dict the trash can
 * @param key the key of the data to be removed
 * @param key_len the size of the @a key
 *
 * Remove the entry associated with the given @a key (throws std::out_of_range)
 */
void xbt_dict_remove_ext(xbt_dict_t dict, const char *key, int key_len)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);
  xbt_dictelm_t previous = nullptr;
  xbt_dictelm_t current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || key_len != current->key_len
          || strncmp(key, current->key, key_len))) {
    previous = current;         /* save the previous node */
    current = current->next;
  }

  if (current == nullptr)
    throw std::out_of_range(simgrid::xbt::string_printf("key %.*s not found", key_len, key));
  else {
    if (previous != nullptr) {
      previous->next = current->next;
    } else {
      dict->table[hash_code & dict->table_size] = current->next;
    }
  }

  if (not dict->table[hash_code & dict->table_size])
    dict->fill--;

  xbt_dictelm_free(dict, current);
  dict->count--;
}

/**
 * @brief Return the number of elements in the dict.
 * @param dict a dictionary
 */
int xbt_dict_length(const_xbt_dict_t dict)
{
  return dict->count;
}

/**
 * @brief test if the dict is empty or not
 */
int xbt_dict_is_empty(const_xbt_dict_t dict)
{
  return not dict || (xbt_dict_length(dict) == 0);
}

/**
 * Create the dict mallocators.
 * This is an internal XBT function called during the lib initialization.
 * It can be used several times to recreate the mallocator, for example when you switch to MC mode
 */
void xbt_dict_preinit()
{
  if (dict_elm_mallocator == nullptr)
    dict_elm_mallocator = xbt_mallocator_new(256, dict_elm_mallocator_new_f, dict_elm_mallocator_free_f,
      dict_elm_mallocator_reset_f);
}

/**
 * Destroy the dict mallocators.
 * This is an internal XBT function during the lib initialization
 */
void xbt_dict_postexit()
{
  if (dict_elm_mallocator != nullptr) {
    xbt_mallocator_free(dict_elm_mallocator);
    dict_elm_mallocator = nullptr;
  }
}
