/* dict - a generic dictionary, variation over hash table                   */

/* Copyright (c) 2004-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdio>
#include <cstring>

#include "xbt/dict.h"
#include "xbt/ex.h"
#include <xbt/ex.hpp>
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "src/xbt_modinter.h"
#include "xbt/str.h"
#include "dict_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict, xbt, "Dictionaries provide the same functionalities as hash tables");

xbt_dict_t xbt_dict_new()
{
  XBT_WARN("Function xbt_dict_new() will soon be dropped. Please switch to xbt_dict_new_homogeneous()");
  xbt_dict_t dict = xbt_dict_new_homogeneous(nullptr);
  dict->homogeneous = 0;

  return dict;
}

/**
 * \brief Constructor
 * \param free_ctn function to call with (\a data as argument) when \a data is removed from the dictionary
 * \return pointer to the destination
 * \see xbt_dict_new(), xbt_dict_free()
 *
 * Creates and initialize a new dictionary with a default hashtable size.
 * The dictionary is homogeneous: each element share the same free function.
 */
xbt_dict_t xbt_dict_new_homogeneous(void_f_pvoid_t free_ctn)
{
  if (dict_elm_mallocator == nullptr)
    xbt_dict_preinit();

  xbt_dict_t dict;

  dict = xbt_new(s_xbt_dict_t, 1);
  dict->free_f = free_ctn;
  dict->table_size = 127;
  dict->table = xbt_new0(xbt_dictelm_t, dict->table_size + 1);
  dict->count = 0;
  dict->fill = 0;
  dict->homogeneous = 1;

  return dict;
}

/**
 * \brief Destructor
 * \param dict the dictionary to be freed
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
unsigned int xbt_dict_size(xbt_dict_t dict)
{
  return (dict != nullptr ? static_cast<unsigned int>(dict->count) : static_cast<unsigned int>(0));
}

/* Expend the size of the dict */
static void xbt_dict_rehash(xbt_dict_t dict)
{
  const unsigned oldsize = dict->table_size + 1;
  unsigned newsize = oldsize * 2;

  xbt_dictelm_t *currcell = (xbt_dictelm_t *) xbt_realloc((char *) dict->table, newsize * sizeof(xbt_dictelm_t));
  memset(&currcell[oldsize], 0, oldsize * sizeof(xbt_dictelm_t));       /* zero second half */
  newsize--;
  dict->table_size = newsize;
  dict->table = currcell;
  XBT_DEBUG("REHASH (%u->%u)", oldsize, newsize);

  for (unsigned i = 0; i < oldsize; i++, currcell++) {
    if (*currcell == nullptr) /* empty cell */
      continue;

    xbt_dictelm_t *twincell = currcell + oldsize;
    xbt_dictelm_t *pprev = currcell;
    xbt_dictelm_t bucklet = *currcell;
    for (; bucklet != nullptr; bucklet = *pprev) {
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
    }

    if (*currcell == nullptr) /* everything moved */
      dict->fill--;
  }
}

/**
 * \brief Add data to the dict (arbitrary key)
 * \param dict the container
 * \param key the key to set the new data
 * \param key_len the size of the \a key
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a data as argument) when \a data is removed from the dictionary. This param
 *        will only be considered when the dict was instantiated with xbt_dict_new() and not xbt_dict_new_homogeneous()
 *
 * Set the \a data in the structure under the \a key, which can be any kind of data, as long as its length is provided
 * in \a key_len.
 */
void xbt_dict_set_ext(xbt_dict_t dict, const char *key, int key_len, void *data, void_f_pvoid_t free_ctn)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);

  xbt_dictelm_t current;
  xbt_dictelm_t previous = nullptr;

  xbt_assert(not free_ctn, "Cannot set an individual free function in homogeneous dicts.");
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
    xbt_dictelm_set_data(dict, current, data, free_ctn);
  }
}

/**
 * \brief Add data to the dict (null-terminated key)
 *
 * \param dict the dict
 * \param key the key to set the new data
 * \param data the data to add in the dict
 * \param free_ctn function to call with (\a data as argument) when \a data is removed from the dictionary. This param
 *        will only be considered when the dict was instantiated with xbt_dict_new() and not xbt_dict_new_homogeneous()
 *
 * set the \a data in the structure under the \a key, which is anull terminated string.
 */
void xbt_dict_set(xbt_dict_t dict, const char *key, void *data, void_f_pvoid_t free_ctn)
{
  xbt_dict_set_ext(dict, key, strlen(key), data, free_ctn);
}

/**
 * \brief Retrieve data from the dict (arbitrary key)
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \param key_len the size of the \a key
 * \return the data that we are looking for
 *
 * Search the given \a key. Throws not_found_error when not found.
 */
void *xbt_dict_get_ext(xbt_dict_t dict, const char *key, int key_len)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);
  xbt_dictelm_t current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || key_len != current->key_len
          || memcmp(key, current->key, key_len))) {
    current = current->next;
  }

  if (current == nullptr)
    THROWF(not_found_error, 0, "key %.*s not found", key_len, key);

  return current->content;
}

/** @brief like xbt_dict_get_ext(), but returning nullptr when not found */
void *xbt_dict_get_or_null_ext(xbt_dict_t dict, const char *key, int key_len)
{
  unsigned int hash_code = xbt_str_hash_ext(key, key_len);
  xbt_dictelm_t current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || key_len != current->key_len
          || memcmp(key, current->key, key_len))) {
    current = current->next;
  }

  if (current == nullptr)
    return nullptr;

  return current->content;
}

/**
 * @brief retrieve the key associated to that object. Warning, that's a linear search
 *
 * Returns nullptr if the object cannot be found
 */
char *xbt_dict_get_key(xbt_dict_t dict, const void *data)
{
  for (int i = 0; i <= dict->table_size; i++) {
    xbt_dictelm_t current = dict->table[i];
    while (current != nullptr) {
      if (current->content == data)
        return current->key;
      current = current->next;
    }
  }
  return nullptr;
}

/** @brief retrieve the key associated to that xbt_dictelm_t. */
char *xbt_dict_get_elm_key(xbt_dictelm_t elm)
{
  return elm->key;
}

/**
 * \brief Retrieve data from the dict (null-terminated key)
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \return the data that we are looking for
 *
 * Search the given \a key. Throws not_found_error when not found.
 * Check xbt_dict_get_or_null() for a version returning nullptr without exception when not found.
 */
void *xbt_dict_get(xbt_dict_t dict, const char *key)
{
  return xbt_dict_get_elm(dict, key)->content;
}

/**
 * \brief Retrieve element from the dict (null-terminated key)
 *
 * \param dict the dealer of data
 * \param key the key to find data
 * \return the s_xbt_dictelm_t that we are looking for
 *
 * Search the given \a key. Throws not_found_error when not found.
 * Check xbt_dict_get_or_null() for a version returning nullptr without exception when not found.
 */
xbt_dictelm_t xbt_dict_get_elm(xbt_dict_t dict, const char *key)
{
  xbt_dictelm_t current = xbt_dict_get_elm_or_null(dict, key);

  if (current == nullptr)
    THROWF(not_found_error, 0, "key %s not found", key);

  return current;
}

/**
 * \brief like xbt_dict_get(), but returning nullptr when not found
 */
void *xbt_dict_get_or_null(xbt_dict_t dict, const char *key)
{
  xbt_dictelm_t current = xbt_dict_get_elm_or_null(dict, key);

  if (current == nullptr)
    return nullptr;

  return current->content;
}

/**
 * \brief like xbt_dict_get_elm(), but returning nullptr when not found
 */
xbt_dictelm_t xbt_dict_get_elm_or_null(xbt_dict_t dict, const char *key)
{
  unsigned int hash_code = xbt_str_hash(key);
  xbt_dictelm_t current = dict->table[hash_code & dict->table_size];

  while (current != nullptr && (hash_code != current->hash_code || strcmp(key, current->key)))
    current = current->next;
  return current;
}

/**
 * \brief Remove data from the dict (arbitrary key)
 *
 * \param dict the trash can
 * \param key the key of the data to be removed
 * \param key_len the size of the \a key
 *
 * Remove the entry associated with the given \a key (throws not_found)
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
    THROWF(not_found_error, 0, "key %.*s not found", key_len, key);
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
 * \brief Remove data from the dict (null-terminated key)
 *
 * \param dict the dict
 * \param key the key of the data to be removed
 *
 * Remove the entry associated with the given \a key
 */
void xbt_dict_remove(xbt_dict_t dict, const char *key)
{
  xbt_dict_remove_ext(dict, key, strlen(key));
}

/** @brief Remove all data from the dict */
void xbt_dict_reset(xbt_dict_t dict)
{
  if (dict->count == 0)
    return;

  for (int i = 0; i <= dict->table_size; i++) {
    xbt_dictelm_t previous = nullptr;
    xbt_dictelm_t current = dict->table[i];
    while (current != nullptr) {
      previous = current;
      current = current->next;
      xbt_dictelm_free(dict, previous);
    }
    dict->table[i] = nullptr;
  }

  dict->count = 0;
  dict->fill = 0;
}

/**
 * \brief Return the number of elements in the dict.
 * \param dict a dictionary
 */
int xbt_dict_length(xbt_dict_t dict)
{
  return dict->count;
}

/** @brief function to be used in xbt_dict_dump as long as the stored values are strings */
void xbt_dict_dump_output_string(void *s)
{
  fputs((char*) s, stdout);
}

/**
 * \brief test if the dict is empty or not
 */
int xbt_dict_is_empty(xbt_dict_t dict)
{
  return not dict || (xbt_dict_length(dict) == 0);
}

/**
 * \brief Outputs the content of the structure (debugging purpose)
 *
 * \param dict the exibitionist
 * \param output a function to dump each data in the tree (check @ref xbt_dict_dump_output_string)
 *
 * Outputs the content of the structure. (for debugging purpose). \a output is a function to output the data. If nullptr,
 * data won't be displayed.
 */
void xbt_dict_dump(xbt_dict_t dict, void_f_pvoid_t output)
{
  xbt_dictelm_t element;
  printf("Dict %p:\n", dict);
  if (dict != nullptr) {
    for (int i = 0; i < dict->table_size; i++) {
      element = dict->table[i];
      if (element) {
        printf("[\n");
        while (element != nullptr) {
          printf(" %s -> '", element->key);
          if (output != nullptr) {
            output(element->content);
          }
          printf("'\n");
          element = element->next;
        }
        printf("]\n");
      } else {
        printf("[]\n");
      }
    }
  }
}

xbt_dynar_t all_sizes = nullptr;
/** @brief shows some debugging info about the bucklet repartition */
void xbt_dict_dump_sizes(xbt_dict_t dict)
{
  unsigned int count;
  unsigned int size;

  printf("Dict %p: %d bucklets, %d used cells (of %d) ", dict, dict->count, dict->fill, dict->table_size);

  if (not dict) {
    printf("\n");
    return;
  }
  xbt_dynar_t sizes = xbt_dynar_new(sizeof(int), nullptr);

  for (int i = 0; i < dict->table_size; i++) {
    xbt_dictelm_t element = dict->table[i];
    size = 0;
    if (element) {
      while (element != nullptr) {
        size++;
        element = element->next;
      }
    }
    if (xbt_dynar_length(sizes) <= size) {
      int prevsize = 1;
      xbt_dynar_set(sizes, size, &prevsize);
    } else {
      int prevsize;
      xbt_dynar_get_cpy(sizes, size, &prevsize);
      prevsize++;
      xbt_dynar_set(sizes, size, &prevsize);
    }
  }
  if (not all_sizes)
    all_sizes = xbt_dynar_new(sizeof(int), nullptr);

  xbt_dynar_foreach(sizes, count, size) {
    /* Copy values of this one into all_sizes */
    int prevcount;
    if (xbt_dynar_length(all_sizes) <= count) {
      prevcount = size;
      xbt_dynar_set(all_sizes, count, &prevcount);
    } else {
      xbt_dynar_get_cpy(all_sizes, count, &prevcount);
      prevcount += size;
      xbt_dynar_set(all_sizes, count, &prevcount);
    }

    /* Report current sizes */
    if (count != 0 && size != 0)
      printf("%uelm x %u cells; ", count, size);
  }
  printf("\n");
  xbt_dynar_free(&sizes);
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
  if (all_sizes) {
    unsigned int count;
    int size;
    double avg = 0;
    int total_count = 0;
    printf("Overall stats:");
    xbt_dynar_foreach(all_sizes, count, size) {
      if (count != 0 && size != 0) {
        printf("%uelm x %d cells; ", count, size);
        avg += count * size;
        total_count += size;
      }
    }
    if (total_count > 0)
      printf("; %f elm per cell\n", avg / (double)total_count);
    else
      printf("; 0 elm per cell\n");
  }
}

#ifdef SIMGRID_TEST
#include "src/internal_config.h"
#include "xbt.h"
#include "xbt/ex.h"
#include <ctime>
#include <xbt/ex.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_dict);

XBT_TEST_SUITE("dict", "Dict data container");

static void debugged_add_ext(xbt_dict_t head, const char* key, const char* data_to_fill)
{
  char *data = xbt_strdup(data_to_fill);

  xbt_test_log("Add %s under %s", data_to_fill, key);

  xbt_dict_set(head, key, data, nullptr);
  if (XBT_LOG_ISENABLED(xbt_dict, xbt_log_priority_debug)) {
    xbt_dict_dump(head, (void (*)(void *)) &printf);
    fflush(stdout);
  }
}

static void debugged_add(xbt_dict_t head, const char* key)
{
  debugged_add_ext(head, key, key);
}

static xbt_dict_t new_fixture()
{
  xbt_test_add("Fill in the dictionnary");

  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  debugged_add(head, "12");
  debugged_add(head, "12a");
  debugged_add(head, "12b");
  debugged_add(head, "123");
  debugged_add(head, "123456");
  debugged_add(head, "1234");
  debugged_add(head, "123457");

  return head;
}

static void search_ext(xbt_dict_t head, const char *key, const char *data)
{
  xbt_test_add("Search %s", key);
  char *found = (char*) xbt_dict_get(head, key);
  xbt_test_log("Found %s", found);
  if (data) {
    xbt_test_assert(found, "data do not match expectations: found nullptr while searching for %s", data);
    if (found)
      xbt_test_assert(not strcmp(data, found), "data do not match expectations: found %s while searching for %s", found,
                      data);
  } else {
    xbt_test_assert(not found, "data do not match expectations: found %s while searching for nullptr", found);
  }
}

static void search(xbt_dict_t head, const char *key)
{
  search_ext(head, key, key);
}

static void debugged_remove(xbt_dict_t head, const char* key)
{
  xbt_test_add("Remove '%s'", key);
  xbt_dict_remove(head, key);
}

static void traverse(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor = nullptr;
  char *key;
  char *data;
  int i = 0;

  xbt_dict_foreach(head, cursor, key, data) {
    if (not key || not data || strcmp(key, data)) {
      xbt_test_log("Seen #%d:  %s->%s", ++i, key, data);
    } else {
      xbt_test_log("Seen #%d:  %s", ++i, key);
    }
    xbt_test_assert(key && data && strcmp(key, data) == 0, "Key(%s) != value(%s). Aborting", key, data);
  }
}

static void search_not_found(xbt_dict_t head, const char *data)
{
  int ok = 0;
  xbt_test_add("Search %s (expected not to be found)", data);

  try {
    data = (const char*) xbt_dict_get(head, data);
    THROWF(unknown_error, 0, "Found something which shouldn't be there (%s)", data);
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    ok = 1;
  }
  xbt_test_assert(ok, "Exception not raised");
}

static void count(xbt_dict_t dict, int length)
{
  xbt_test_add("Count elements (expecting %d)", length);
  xbt_test_assert(xbt_dict_length(dict) == length, "Announced length(%d) != %d.", xbt_dict_length(dict), length);

  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int effective = 0;
  xbt_dict_foreach(dict, cursor, key, data)
      effective++;

  xbt_test_assert(effective == length, "Effective length(%d) != %d.", effective, length);
}

static void count_check_get_key(xbt_dict_t dict, int length)
{
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int effective = 0;

  xbt_test_add("Count elements (expecting %d), and test the getkey function", length);
  xbt_test_assert(xbt_dict_length(dict) == length, "Announced length(%d) != %d.", xbt_dict_length(dict), length);

  xbt_dict_foreach(dict, cursor, key, data) {
    effective++;
    char* key2 = xbt_dict_get_key(dict, data);
    xbt_assert(not strcmp(key, key2), "The data was registered under %s instead of %s as expected", key2, key);
  }

  xbt_test_assert(effective == length, "Effective length(%d) != %d.", effective, length);
}

XBT_TEST_UNIT("basic", test_dict_basic, "Basic usage: change, retrieve and traverse homogeneous dicts")
{
  xbt_test_add("Traversal the null dictionary");
  traverse(nullptr);

  xbt_test_add("Traversal and search the empty dictionary");
  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  traverse(head);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  xbt_dict_free(&head);

  xbt_test_add("Traverse the full dictionary");
  head = new_fixture();
  count_check_get_key(head, 7);

  debugged_add_ext(head, "toto", "tutu");
  search_ext(head, "toto", "tutu");
  debugged_remove(head, "toto");

  search(head, "12a");
  traverse(head);

  xbt_test_add("Free the dictionary (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  /* CHANGING */
  head = new_fixture();
  count_check_get_key(head, 7);
  xbt_test_add("Change 123 to 'Changed 123'");
  xbt_dict_set(head, "123", xbt_strdup("Changed 123"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 123 back to '123'");
  xbt_dict_set(head, "123", xbt_strdup("123"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 12a to 'Dummy 12a'");
  xbt_dict_set(head, "12a", xbt_strdup("Dummy 12a"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 12a to '12a'");
  xbt_dict_set(head, "12a", xbt_strdup("12a"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);

  /* RETRIEVE */
  xbt_test_add("Search 123");
  char* data = (char*)xbt_dict_get(head, "123");
  xbt_test_assert(data && strcmp("123", data) == 0);

  search_not_found(head, "Can't be found");
  search_not_found(head, "123 Can't be found");
  search_not_found(head, "12345678 NOT");

  search(head, "12a");
  search(head, "12b");
  search(head, "12");
  search(head, "123456");
  search(head, "1234");
  search(head, "123457");

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);

  xbt_test_add("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);
}

XBT_TEST_UNIT("remove_homogeneous", test_dict_remove, "Removing some values from homogeneous dicts")
{
  xbt_dict_t head = new_fixture();
  count(head, 7);
  xbt_test_add("Remove non existing data");
  try {
    debugged_remove(head, "Does not exist");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  traverse(head);

  xbt_dict_free(&head);

  xbt_test_add("Remove each data manually (traversing the resulting dictionary each time)");
  head = new_fixture();
  debugged_remove(head, "12a");
  traverse(head);
  count(head, 6);
  debugged_remove(head, "12b");
  traverse(head);
  count(head, 5);
  debugged_remove(head, "12");
  traverse(head);
  count(head, 4);
  debugged_remove(head, "123456");
  traverse(head);
  count(head, 3);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    traverse(head);
  }
  debugged_remove(head, "1234");
  traverse(head);
  debugged_remove(head, "123457");
  traverse(head);
  debugged_remove(head, "123");
  traverse(head);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  traverse(head);

  xbt_test_add("Free dict, create new fresh one, and then reset the dict");
  xbt_dict_free(&head);
  head = new_fixture();
  xbt_dict_reset(head);
  count(head, 0);
  traverse(head);

  xbt_test_add("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

XBT_TEST_UNIT("nulldata", test_dict_nulldata, "nullptr data management")
{
  xbt_dict_t head = new_fixture();

  xbt_test_add("Store nullptr under 'null'");
  xbt_dict_set(head, "null", nullptr, nullptr);
  search_ext(head, "null", nullptr);

  xbt_test_add("Check whether I see it while traversing...");
  {
    xbt_dict_cursor_t cursor = nullptr;
    char *key;
    int found = 0;
    char* data;

    xbt_dict_foreach(head, cursor, key, data) {
      if (not key || not data || strcmp(key, data)) {
        xbt_test_log("Seen:  %s->%s", key, data);
      } else {
        xbt_test_log("Seen:  %s", key);
      }

      if (key && strcmp(key, "null") == 0)
        found = 1;
    }
    xbt_test_assert(found, "the key 'null', associated to nullptr is not found");
  }
  xbt_dict_free(&head);
}

#define NB_ELM 20000
#define SIZEOFKEY 1024
static int countelems(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int res = 0;

  xbt_dict_foreach(head, cursor, key, data) {
    res++;
  }
  return res;
}

XBT_TEST_UNIT("crash", test_dict_crash, "Crash test")
{
  srand((unsigned int) time(nullptr));

  for (int i = 0; i < 10; i++) {
    xbt_test_add("CRASH test number %d (%d to go)", i + 1, 10 - i - 1);
    xbt_test_log("Fill the struct, count its elems and frees the structure");
    xbt_test_log("using 1000 elements with %d chars long randomized keys.", SIZEOFKEY);
    xbt_dict_t head = xbt_dict_new_homogeneous(free);
    for (int j = 0; j < 1000; j++) {
      char* data = nullptr;
      char* key  = (char*)xbt_malloc(SIZEOFKEY);

      do {
        for (int k         = 0; k < SIZEOFKEY - 1; k++)
          key[k] = rand() % ('z' - 'a') + 'a';
        key[SIZEOFKEY - 1] = '\0';
        data = (char*) xbt_dict_get_or_null(head, key);
      } while (data != nullptr);

      xbt_dict_set(head, key, key, nullptr);
      data = (char*) xbt_dict_get(head, key);
      xbt_test_assert(not strcmp(key, data), "Retrieved value (%s) != Injected value (%s)", key, data);

      count(head, j + 1);
    }
    traverse(head);
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }

  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  xbt_test_add("Fill %d elements, with keys being the number of element", NB_ELM);
  for (int j = 0; j < NB_ELM; j++) {
    char* key = (char*)xbt_malloc(10);

    snprintf(key,10, "%d", j);
    xbt_dict_set(head, key, key, nullptr);
  }

  xbt_test_add("Count the elements (retrieving the key and data for each)");
  xbt_test_log("There is %d elements", countelems(head));

  xbt_test_add("Search my %d elements 20 times", NB_ELM);
  char* key = (char*)xbt_malloc(10);
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < NB_ELM; j++) {
      snprintf(key,10, "%d", j);
      void* data = xbt_dict_get(head, key);
      xbt_test_assert(not strcmp(key, (char*)data), "with get, key=%s != data=%s", key, (char*)data);
      data = xbt_dict_get_ext(head, key, strlen(key));
      xbt_test_assert(not strcmp(key, (char*)data), "with get_ext, key=%s != data=%s", key, (char*)data);
    }
  }
  free(key);

  xbt_test_add("Remove my %d elements", NB_ELM);
  key = (char*) xbt_malloc(10);
  for (int j = 0; j < NB_ELM; j++) {
    snprintf(key,10, "%d", j);
    xbt_dict_remove(head, key);
  }
  free(key);

  xbt_test_add("Free the object (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

XBT_TEST_UNIT("ext", test_dict_int, "Test dictionnary with int keys")
{
  xbt_dict_t dict = xbt_dict_new_homogeneous(nullptr);
  int count = 500;

  xbt_test_add("Insert elements");
  for (int i = 0; i < count; ++i)
    xbt_dict_set_ext(dict, (char*) &i, sizeof(i), (void*) (intptr_t) i, nullptr);
  xbt_test_assert(xbt_dict_size(dict) == (unsigned) count, "Bad number of elements in the dictionnary");

  xbt_test_add("Check elements");
  for (int i = 0; i < count; ++i) {
    int res = (int) (intptr_t) xbt_dict_get_ext(dict, (char*) &i, sizeof(i));
    xbt_test_assert(xbt_dict_size(dict) == (unsigned) count, "Unexpected value at index %i, expected %i but was %i", i, i, res);
  }

  xbt_test_add("Free the array");
  xbt_dict_free(&dict);
}
#endif                          /* SIMGRID_TEST */
