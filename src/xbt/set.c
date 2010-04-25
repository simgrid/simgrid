/* set - data container consisting in dict+dynar                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "xbt/set.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_set, xbt,
                                "set: data container consisting in dict+dynar");

/*####[ Type definition ]####################################################*/
typedef struct xbt_set_ {
  xbt_dict_t dict;              /* data stored by name */
  xbt_dynar_t dynar;            /* data stored by ID   */
  xbt_dynar_t available_ids;    /* free places in the dynar */
} s_xbt_set_t;

/*####[ Memory  ]############################################################*/

static int _xbt_set_get_id(xbt_set_t set);

/** @brief Constructor */
xbt_set_t xbt_set_new(void)
{
  xbt_set_t res = xbt_new(s_xbt_set_t, 1);

  res->dict = xbt_dict_new();
  res->dynar = xbt_dynar_new(sizeof(void *), NULL);
  res->available_ids = xbt_dynar_new(sizeof(int), NULL);

  return res;
}

/** @brief Destructor */
void xbt_set_free(xbt_set_t * set)
{
  if (*set) {
    xbt_dict_free(&((*set)->dict));
    xbt_dynar_free(&((*set)->dynar));
    xbt_dynar_free(&((*set)->available_ids));
    free(*set);
    *set = NULL;
  }
}

/* Compute an ID in order to add an element into the set. */
static int _xbt_set_get_id(xbt_set_t set)
{
  int id;
  if (xbt_dynar_length(set->available_ids) > 0) {
    /* if there are some available ids */
    xbt_dynar_pop(set->available_ids, &id);
  } else {
    /* otherwise we will add the element at the dynar end */
    id = xbt_dynar_length(set->dynar);
  }
  return id;
}

/** @brief Add an element to a set.
 *
 * \param set set to populate
 * \param elm element to add.
 * \param free_func How to add the data
 *
 * elm->name must be set;
 * if elm->name_len <= 0, it is recomputed. If >0, it's used as is;
 * elm->ID is attributed automatically.
 */
void xbt_set_add(xbt_set_t set, xbt_set_elm_t elm, void_f_pvoid_t free_func)
{

  int found = 1;
  xbt_set_elm_t found_in_dict = NULL;
  xbt_ex_t e;

  VERB1("add %s to the set", elm->name);

  if (elm->name_len <= 0) {
    elm->name_len = strlen(elm->name);
  }

  TRY {
    found_in_dict = xbt_dict_get_ext(set->dict, elm->name, elm->name_len);
  }
  CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    found = 0;
    elm->ID = _xbt_set_get_id(set);
    xbt_dict_set_ext(set->dict, elm->name, elm->name_len, elm, free_func);
    xbt_dynar_set(set->dynar, elm->ID, &elm);
    DEBUG2("Insertion of key '%s' (id %d)", elm->name, elm->ID);
    xbt_ex_free(e);
  }

  if (found) {
    if (elm == found_in_dict) {
      DEBUG2
        ("Ignoring request to insert the same element twice (key %s ; id %d)",
         elm->name, elm->ID);
      return;
    } else {
      elm->ID = found_in_dict->ID;
      DEBUG2("Reinsertion of key %s (id %d)", elm->name, elm->ID);
      xbt_dict_set_ext(set->dict, elm->name, elm->name_len, elm, free_func);
      xbt_dynar_set(set->dynar, elm->ID, &elm);
      return;
    }
  }
}

/** @brief Remove an element from a set.
 *
 * \param set a set
 * \param elm element to remove
 */
void xbt_set_remove(xbt_set_t set, xbt_set_elm_t elm)
{
  int id = elm->ID;
  xbt_dynar_push_as(set->available_ids, int, id);       /* this id becomes available now */
  xbt_dict_remove_ext(set->dict, elm->name, elm->name_len);
  elm = NULL;
  xbt_dynar_set(set->dynar, id, &elm);
}

/** @brief Remove an element from a set providing its name.
 *
 * \param set a set
 * \param key name of the element to remove
 */
void xbt_set_remove_by_name(xbt_set_t set, const char *key)
{
  xbt_set_elm_t elm = xbt_set_get_by_name(set, key);
  xbt_set_remove(set, elm);
}

/** @brief Remove an element from a set providing its name
 * and the length of the name.
 *
 * \param set a set
 * \param key name of the element to remove
 * \param key_len length of \a name
 */
void xbt_set_remove_by_name_ext(xbt_set_t set, const char *key, int key_len)
{
  xbt_set_elm_t elm = xbt_set_get_by_name_ext(set, key, key_len);
  xbt_set_remove(set, elm);
}

/** @brief Remove an element from a set providing its id.
 *
 * \param set a set
 * \param id id of the element to remove
 */
void xbt_set_remove_by_id(xbt_set_t set, int id)
{
  xbt_set_elm_t elm = xbt_set_get_by_id(set, id);
  xbt_set_remove(set, elm);
}

/** @brief Retrieve data by providing its name.
 *
 * \param set
 * \param name Name of the searched cell
 * \returns the data you're looking for
 */
xbt_set_elm_t xbt_set_get_by_name(xbt_set_t set, const char *name)
{
  DEBUG1("Lookup key %s", name);
  return xbt_dict_get(set->dict, name);
}

/** @brief Retrieve data by providing its name.
 *
 * \param set
 * \param name Name of the searched cell
 * \returns the data you're looking for, returns NULL if not found
 */
xbt_set_elm_t xbt_set_get_by_name_or_null(xbt_set_t set, const char *name)
{
  DEBUG1("Lookup key %s", name);
  return xbt_dict_get_or_null(set->dict, name);
}

/** @brief Retrieve data by providing its name and the length of the name
 *
 * \param set
 * \param name Name of the searched cell
 * \param name_len length of the name, when strlen cannot be trusted
 * \returns the data you're looking for
 *
 * This is useful when strlen cannot be trusted because you don't use a char*
 * as name, you weirdo.
 */
xbt_set_elm_t xbt_set_get_by_name_ext(xbt_set_t set,
                                      const char *name, int name_len)
{

  return xbt_dict_get_ext(set->dict, name, name_len);
}

/** @brief Retrieve data by providing its ID
 *
 * \param set
 * \param id what you're looking for
 * \returns the data you're looking for
 *
 * @warning, if the ID does not exists, you're getting into trouble
 */
xbt_set_elm_t xbt_set_get_by_id(xbt_set_t set, int id)
{
  xbt_set_elm_t res;

  /* Don't bother checking the bounds, the dynar does so */

  res = xbt_dynar_get_as(set->dynar, id, xbt_set_elm_t);
  if (res == NULL) {
    THROW1(not_found_error, 0, "Invalid id: %d", id);
  }
  DEBUG3("Lookup type of id %d (of %lu): %s",
         id, xbt_dynar_length(set->dynar), res->name);

  return res;
}

/**
 * \brief Returns the number of elements in the set
 * \param set a set
 * \return the number of elements in the set
 */
unsigned long xbt_set_length(const xbt_set_t set)
{
  return xbt_dynar_length(set->dynar);
}

/***
 *** Cursors
 ***/
typedef struct xbt_set_cursor_ {
  xbt_set_t set;
  unsigned int val;
} s_xbt_set_cursor_t;

/** @brief Create the cursor if it does not exists, rewind it in any case. */
void xbt_set_cursor_first(xbt_set_t set, xbt_set_cursor_t * cursor)
{
  xbt_dynar_t dynar;

  if (set != NULL) {
    if (!*cursor) {
      DEBUG0("Create the cursor on first use");
      *cursor = xbt_new(s_xbt_set_cursor_t, 1);
      xbt_assert0(*cursor, "Malloc error during the creation of the cursor");
    }
    (*cursor)->set = set;

    /* place the cursor on the first element */
    dynar = set->dynar;
    (*cursor)->val = 0;
    while (xbt_dynar_get_ptr(dynar, (*cursor)->val) == NULL) {
      (*cursor)->val++;
    }

  } else {
    *cursor = NULL;
  }
}

/** @brief Move to the next element.  */
void xbt_set_cursor_step(xbt_set_cursor_t cursor)
{
  xbt_dynar_t dynar = cursor->set->dynar;
  do {
    cursor->val++;
  }
  while (cursor->val < xbt_dynar_length(dynar) &&
         xbt_dynar_get_ptr(dynar, cursor->val) == NULL);
}

/** @brief Get current data
 *
 * \return true if it's ok, false if there is no more data
 */
int xbt_set_cursor_get_or_free(xbt_set_cursor_t * curs, xbt_set_elm_t * elm)
{
  xbt_set_cursor_t cursor;

  if (!curs || !(*curs))
    return FALSE;

  cursor = *curs;

  if (cursor->val >= xbt_dynar_length(cursor->set->dynar)) {
    free(cursor);
    *curs = NULL;
    return FALSE;
  }

  xbt_dynar_get_cpy(cursor->set->dynar, cursor->val, elm);
  return TRUE;
}

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"

XBT_TEST_SUITE("set", "Set data container");

typedef struct {
  /* headers */
  unsigned int ID;
  char *name;
  unsigned int name_len;

  /* payload */
  char *data;
} s_my_elem_t, *my_elem_t;


static void my_elem_free(void *e)
{
  my_elem_t elm = (my_elem_t) e;

  if (elm) {
    free(elm->name);
    free(elm->data);
    free(elm);
  }
}

static void debuged_add(xbt_set_t set, const char *name, const char *data)
{
  my_elem_t elm;

  elm = xbt_new(s_my_elem_t, 1);
  elm->name = xbt_strdup(name);
  elm->name_len = 0;

  elm->data = xbt_strdup(data);

  xbt_test_log2("Add %s (->%s)", name, data);
  xbt_set_add(set, (xbt_set_elm_t) elm, &my_elem_free);
}

static void fill(xbt_set_t * set)
{
  xbt_test_add0("Fill in the data set");

  *set = xbt_set_new();
  debuged_add(*set, "12", "12");
  debuged_add(*set, "12a", "12a");
  debuged_add(*set, "12b", "12b");
  debuged_add(*set, "123", "123");
  debuged_add(*set, "123456", "123456");
  xbt_test_log0("Child becomes child of what to add");
  debuged_add(*set, "1234", "1234");
  xbt_test_log0("Need of common ancestor");
  debuged_add(*set, "123457", "123457");
}

static void search_name(xbt_set_t head, const char *key)
{
  my_elem_t elm;

  xbt_test_add1("Search by name %s", key);
  elm = (my_elem_t) xbt_set_get_by_name(head, key);
  xbt_test_log2(" Found %s (under ID %d)\n",
                elm ? elm->data : "(null)", elm ? elm->ID : -1);
  if (strcmp(key, elm->name))
    THROW2(mismatch_error, 0, "The key (%s) is not the one expected (%s)",
           key, elm->name);
  if (strcmp(elm->name, elm->data))
    THROW2(mismatch_error, 0, "The name (%s) != data (%s)", key, elm->name);
  fflush(stdout);
}

static void search_id(xbt_set_t head, int id, const char *key)
{
  my_elem_t elm;

  xbt_test_add1("Search by id %d", id);
  elm = (my_elem_t) xbt_set_get_by_id(head, id);
  xbt_test_log2("Found %s (data %s)",
                elm ? elm->name : "(null)", elm ? elm->data : "(null)");
  if (id != elm->ID)
    THROW2(mismatch_error, 0,
           "The found ID (%d) is not the one expected (%d)", elm->ID, id);
  if (strcmp(key, elm->name))
    THROW2(mismatch_error, 0, "The key (%s) is not the one expected (%s)",
           elm->name, key);
  if (strcmp(elm->name, elm->data))
    THROW2(mismatch_error, 0, "The name (%s) != data (%s)",
           elm->name, elm->data);
}


static void traverse(xbt_set_t set)
{
  xbt_set_cursor_t cursor = NULL;
  my_elem_t elm = NULL;

  xbt_set_foreach(set, cursor, elm) {
    xbt_test_assert0(elm, "Dude ! Got a null elm during traversal!");
    xbt_test_log3("Id(%d):  %s->%s\n", elm->ID, elm->name, elm->data);
    xbt_test_assert2(!strcmp(elm->name, elm->data),
                     "Key(%s) != value(%s). Abording", elm->name, elm->data);
  }
}

static void search_not_found(xbt_set_t set, const char *data)
{
  xbt_ex_t e;

  xbt_test_add1("Search %s (expected not to be found)", data);
  TRY {
    xbt_set_get_by_name(set, data);
    THROW1(unknown_error, 0, "Found something which shouldn't be there (%s)",
           data);
  } CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
  }
}

xbt_set_t set = NULL;


XBT_TEST_UNIT("basic", test_set_basic, "Basic usage")
{
  set = NULL;

  xbt_test_add0("Traverse the empty set");
  traverse(set);

  xbt_test_add0("Free a data set");
  fill(&set);
  xbt_set_free(&set);

  xbt_test_add0("Free the NULL data set");
  xbt_set_free(&set);

}

XBT_TEST_UNIT("change", test_set_change, "Changing some values")
{
  fill(&set);

  xbt_test_add0("Change 123 to 'Changed 123'");
  debuged_add(set, "123", "Changed 123");

  xbt_test_add0("Change 123 back to '123'");
  debuged_add(set, "123", "123");

  xbt_test_add0("Change 12a to 'Dummy 12a'");
  debuged_add(set, "12a", "Dummy 12a");

  xbt_test_add0("Change 12a to '12a'");
  debuged_add(set, "12a", "12a");

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
  xbt_test_add0("Traverse the resulting data set");
  traverse(set);
}

XBT_TEST_UNIT("retrieve", test_set_retrieve, "Retrieving some values")
{
  my_elem_t elm;

  xbt_test_add0("Search 123");
  elm = (my_elem_t) xbt_set_get_by_name(set, "123");
  xbt_test_assert0(elm, "elm must be there");
  xbt_assert(!strcmp("123", elm->data));

  search_not_found(set, "Can't be found");
  search_not_found(set, "123 Can't be found");
  search_not_found(set, "12345678 NOT");

  search_name(set, "12");
  search_name(set, "12a");
  search_name(set, "12b");
  search_name(set, "123");
  search_name(set, "123456");
  search_name(set, "1234");
  search_name(set, "123457");

  search_id(set, 0, "12");
  search_id(set, 1, "12a");
  search_id(set, 2, "12b");
  search_id(set, 3, "123");
  search_id(set, 4, "123456");
  search_id(set, 5, "1234");
  search_id(set, 6, "123457");

  xbt_test_add0("Traverse the resulting data set");
  traverse(set);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  xbt_test_add0("Free the data set (twice)");
  xbt_set_free(&set);
  xbt_set_free(&set);

  xbt_test_add0("Traverse the resulting data set");
  traverse(set);
}

XBT_TEST_UNIT("remove", test_set_remove, "Removing some values")
{
  my_elem_t elm;

  xbt_set_free(&set);
  fill(&set);

  xbt_set_remove_by_name(set, "12a");
  search_not_found(set, "12a");

  search_name(set, "12");
  search_name(set, "12b");
  search_name(set, "123");
  search_name(set, "123456");
  search_name(set, "1234");
  search_name(set, "123457");

  search_id(set, 0, "12");
  search_id(set, 2, "12b");
  search_id(set, 3, "123");
  search_id(set, 4, "123456");
  search_id(set, 5, "1234");
  search_id(set, 6, "123457");

  debuged_add(set, "12anew", "12anew");
  elm = (my_elem_t) xbt_set_get_by_id(set, 1);
  xbt_test_assert1(elm->ID == 1, "elm->ID is %d but should be 1", elm->ID);
}

#endif /* SIMGRID_TEST */
