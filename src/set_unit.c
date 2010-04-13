/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

# 317 "/home/navarrop/Developments/simgrid/src/xbt/set.c" 
#include "xbt.h"
#include "xbt/ex.h"


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

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

