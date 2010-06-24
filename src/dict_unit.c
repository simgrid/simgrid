/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 810 "xbt/dict.c" 
#include "xbt.h"
#include "xbt/ex.h"
#include "portable.h"

XBT_LOG_EXTERNAL_CATEGORY(xbt_dict);
XBT_LOG_DEFAULT_CATEGORY(xbt_dict);

static void print_str(void *str)
{
  printf("%s", (char *) PRINTF_STR(str));
}

static void debuged_add_ext(xbt_dict_t head, const char *key,
                            const char *data_to_fill)
{
  char *data = xbt_strdup(data_to_fill);

  xbt_test_log2("Add %s under %s", PRINTF_STR(data_to_fill), PRINTF_STR(key));

  xbt_dict_set(head, key, data, &free);
  if (XBT_LOG_ISENABLED(xbt_dict, xbt_log_priority_debug)) {
    xbt_dict_dump(head, (void (*)(void *)) &printf);
    fflush(stdout);
  }
}

static void debuged_add(xbt_dict_t head, const char *key)
{
  debuged_add_ext(head, key, key);
}

static void fill(xbt_dict_t * head)
{
  xbt_test_add0("Fill in the dictionnary");

  *head = xbt_dict_new();
  debuged_add(*head, "12");
  debuged_add(*head, "12a");
  debuged_add(*head, "12b");
  debuged_add(*head, "123");
  debuged_add(*head, "123456");
  /* Child becomes child of what to add */
  debuged_add(*head, "1234");
  /* Need of common ancestor */
  debuged_add(*head, "123457");
}


static void search_ext(xbt_dict_t head, const char *key, const char *data)
{
  void *found;

  xbt_test_add1("Search %s", key);
  found = xbt_dict_get(head, key);
  xbt_test_log1("Found %s", (char *) found);
  if (data)
    xbt_test_assert1(found,
                     "data do not match expectations: found NULL while searching for %s",
                     data);
  if (found)
    xbt_test_assert2(!strcmp((char *) data, found),
                     "data do not match expectations: found %s while searching for %s",
                     (char *) found, data);
}

static void search(xbt_dict_t head, const char *key)
{
  search_ext(head, key, key);
}

static void debuged_remove(xbt_dict_t head, const char *key)
{

  xbt_test_add1("Remove '%s'", PRINTF_STR(key));
  xbt_dict_remove(head, key);
  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
}


static void traverse(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  char *data;
  int i = 0;

  xbt_dict_foreach(head, cursor, key, data) {
    if (!key || !data || strcmp(key, data)) {
      xbt_test_log3("Seen #%d:  %s->%s", ++i, PRINTF_STR(key),
                    PRINTF_STR(data));
    } else {
      xbt_test_log2("Seen #%d:  %s", ++i, PRINTF_STR(key));
    }
    xbt_test_assert2(!data || !strcmp(key, data),
                     "Key(%s) != value(%s). Aborting", key, data);
  }
}

static void search_not_found(xbt_dict_t head, const char *data)
{
  int ok = 0;
  xbt_ex_t e;

  xbt_test_add1("Search %s (expected not to be found)", data);

  TRY {
    data = xbt_dict_get(head, data);
    THROW1(unknown_error, 0, "Found something which shouldn't be there (%s)",
           data);
  } CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
    ok = 1;
  }
  xbt_test_assert0(ok, "Exception not raised");
}

static void count(xbt_dict_t dict, int length)
{
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int effective = 0;


  xbt_test_add1("Count elements (expecting %d)", length);
  xbt_test_assert2(xbt_dict_length(dict) == length,
                   "Announced length(%d) != %d.", xbt_dict_length(dict),
                   length);

  xbt_dict_foreach(dict, cursor, key, data)
    effective++;

  xbt_test_assert2(effective == length, "Effective length(%d) != %d.",
                   effective, length);

}

static void count_check_get_key(xbt_dict_t dict, int length)
{
  xbt_dict_cursor_t cursor;
  char *key,*key2;
  void *data;
  int effective = 0;


  xbt_test_add1("Count elements (expecting %d), and test the getkey function", length);
  xbt_test_assert2(xbt_dict_length(dict) == length,
                   "Announced length(%d) != %d.", xbt_dict_length(dict),
                   length);

  xbt_dict_foreach(dict, cursor, key, data) {
    effective++;
    key2 = xbt_dict_get_key(dict,data);
    xbt_assert2(!strcmp(key,key2),
          "The data was registered under %s instead of %s as expected",key2,key);
  }

  xbt_test_assert2(effective == length, "Effective length(%d) != %d.",
                   effective, length);

}

xbt_ex_t e;
xbt_dict_t head = NULL;
char *data;


XBT_TEST_UNIT("basic", test_dict_basic,"Basic usage: change, retrieve, traverse")
{
  xbt_test_add0("Traversal the null dictionary");
  traverse(head);

  xbt_test_add0("Traversal and search the empty dictionary");
  head = xbt_dict_new();
  traverse(head);
  TRY {
    debuged_remove(head, "12346");
  } CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
  }
  xbt_dict_free(&head);

  xbt_test_add0("Traverse the full dictionary");
  fill(&head);
  count_check_get_key(head, 7);

  debuged_add_ext(head, "toto", "tutu");
  search_ext(head, "toto", "tutu");
  debuged_remove(head, "toto");

  search(head, "12a");
  traverse(head);

  xbt_test_add0("Free the dictionary (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  /* CHANGING */
  fill(&head);
  count_check_get_key(head, 7);
  xbt_test_add0("Change 123 to 'Changed 123'");
  xbt_dict_set(head, "123", strdup("Changed 123"), &free);
  count_check_get_key(head, 7);

  xbt_test_add0("Change 123 back to '123'");
  xbt_dict_set(head, "123", strdup("123"), &free);
  count_check_get_key(head, 7);

  xbt_test_add0("Change 12a to 'Dummy 12a'");
  xbt_dict_set(head, "12a", strdup("Dummy 12a"), &free);
  count_check_get_key(head, 7);

  xbt_test_add0("Change 12a to '12a'");
  xbt_dict_set(head, "12a", strdup("12a"), &free);
  count_check_get_key(head, 7);

  xbt_test_add0("Traverse the resulting dictionary");
  traverse(head);

  /* RETRIEVE */
  xbt_test_add0("Search 123");
  data = xbt_dict_get(head, "123");
  xbt_test_assert(data);
  xbt_test_assert(!strcmp("123", data));

  search_not_found(head, "Can't be found");
  search_not_found(head, "123 Can't be found");
  search_not_found(head, "12345678 NOT");

  search(head, "12a");
  search(head, "12b");
  search(head, "12");
  search(head, "123456");
  search(head, "1234");
  search(head, "123457");

  xbt_test_add0("Traverse the resulting dictionary");
  traverse(head);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  xbt_test_add0("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  xbt_test_add0("Traverse the resulting dictionary");
  traverse(head);
}

XBT_TEST_UNIT("remove", test_dict_remove, "Removing some values")
{
  fill(&head);
  count(head, 7);
  xbt_test_add0("Remove non existing data");
  TRY {
    debuged_remove(head, "Does not exist");
  }
  CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
  }
  traverse(head);

  xbt_dict_free(&head);

  xbt_test_add0
    ("Remove each data manually (traversing the resulting dictionary each time)");
  fill(&head);
  debuged_remove(head, "12a");
  traverse(head);
  count(head, 6);
  debuged_remove(head, "12b");
  traverse(head);
  count(head, 5);
  debuged_remove(head, "12");
  traverse(head);
  count(head, 4);
  debuged_remove(head, "123456");
  traverse(head);
  count(head, 3);
  TRY {
    debuged_remove(head, "12346");
  }
  CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
    traverse(head);
  }
  debuged_remove(head, "1234");
  traverse(head);
  debuged_remove(head, "123457");
  traverse(head);
  debuged_remove(head, "123");
  traverse(head);
  TRY {
    debuged_remove(head, "12346");
  }
  CATCH(e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    xbt_ex_free(e);
  }
  traverse(head);

  xbt_test_add0("Free dict, create new fresh one, and then reset the dict");
  xbt_dict_free(&head);
  fill(&head);
  xbt_dict_reset(head);
  count(head, 0);
  traverse(head);

  xbt_test_add0("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

XBT_TEST_UNIT("nulldata", test_dict_nulldata, "NULL data management")
{
  fill(&head);

  xbt_test_add0("Store NULL under 'null'");
  xbt_dict_set(head, "null", NULL, NULL);
  search_ext(head, "null", NULL);

  xbt_test_add0("Check whether I see it while traversing...");
  {
    xbt_dict_cursor_t cursor = NULL;
    char *key;
    int found = 0;

    xbt_dict_foreach(head, cursor, key, data) {
      if (!key || !data || strcmp(key, data)) {
        xbt_test_log2("Seen:  %s->%s", PRINTF_STR(key), PRINTF_STR(data));
      } else {
        xbt_test_log1("Seen:  %s", PRINTF_STR(key));
      }

      if (!strcmp(key, "null"))
        found = 1;
    }
    xbt_test_assert0(found,
                     "the key 'null', associated to NULL is not found");
  }
  xbt_dict_free(&head);
}

static void debuged_addi(xbt_dict_t head, uintptr_t key, uintptr_t data) {
	uintptr_t stored_data = 0;
  xbt_test_log2("Add %zu under %zu", data, key);

  xbt_dicti_set(head, key, data);
  if (XBT_LOG_ISENABLED(xbt_dict, xbt_log_priority_debug)) {
    xbt_dict_dump(head, (void (*)(void *)) &printf);
    fflush(stdout);
  }
  stored_data = xbt_dicti_get(head, key);
  xbt_test_assert3(stored_data==data,
      "Retrieved data (%zu) is not what I just stored (%zu) under key %zu",stored_data,data,key);
}

XBT_TEST_UNIT("dicti", test_dict_scalar, "Scalar data and key management")
{
  xbt_test_add0("Fill in the dictionnary");

  head = xbt_dict_new();
  debuged_addi(head, 12, 12);
  debuged_addi(head, 13, 13);
  debuged_addi(head, 14, 14);
  debuged_addi(head, 15, 15);
  /* Change values */
  debuged_addi(head, 12, 15);
  debuged_addi(head, 15, 2000);
  debuged_addi(head, 15, 3000);
  /* 0 as key */
  debuged_addi(head, 0, 1000);
  debuged_addi(head, 0, 2000);
  debuged_addi(head, 0, 3000);
  /* 0 as value */
  debuged_addi(head, 12, 0);
  debuged_addi(head, 13, 0);
  debuged_addi(head, 12, 0);
  debuged_addi(head, 0, 0);

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
  xbt_dict_t head = NULL;
  int i, j, k, nb;
  char *key;
  void *data;

  srand((unsigned int) time(NULL));

  for (i = 0; i < 10; i++) {
    xbt_test_add2("CRASH test number %d (%d to go)", i + 1, 10 - i - 1);
    xbt_test_log0("Fill the struct, count its elems and frees the structure");
    xbt_test_log1("using 1000 elements with %d chars long randomized keys.",
                  SIZEOFKEY);
    head = xbt_dict_new();
    /* if (i%10) printf("."); else printf("%d",i/10); fflush(stdout); */
    nb = 0;
    for (j = 0; j < 1000; j++) {
      char *data = NULL;
      key = xbt_malloc(SIZEOFKEY);

      do {
        for (k = 0; k < SIZEOFKEY - 1; k++)
          key[k] = rand() % ('z' - 'a') + 'a';
        key[k] = '\0';
        /*      printf("[%d %s]\n",j,key); */
        data = xbt_dict_get_or_null(head, key);
      } while (data != NULL);

      xbt_dict_set(head, key, key, &free);
      data = xbt_dict_get(head, key);
      xbt_test_assert2(!strcmp(key, data),
                       "Retrieved value (%s) != Injected value (%s)", key,
                       data);

      count(head, j + 1);
    }
    /*    xbt_dict_dump(head,(void (*)(void*))&printf); */
    traverse(head);
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }


  head = xbt_dict_new();
  xbt_test_add1("Fill %d elements, with keys being the number of element",
                NB_ELM);
  for (j = 0; j < NB_ELM; j++) {
    /* if (!(j%1000)) { printf("."); fflush(stdout); } */

    key = xbt_malloc(10);

    sprintf(key, "%d", j);
    xbt_dict_set(head, key, key, &free);
  }
  /*xbt_dict_dump(head,(void (*)(void*))&printf); */

  xbt_test_add0("Count the elements (retrieving the key and data for each)");
  i = countelems(head);
  xbt_test_log1("There is %d elements", i);

  xbt_test_add1("Search my %d elements 20 times", NB_ELM);
  key = xbt_malloc(10);
  for (i = 0; i < 20; i++) {
    /* if (i%10) printf("."); else printf("%d",i/10); fflush(stdout); */
    for (j = 0; j < NB_ELM; j++) {

      sprintf(key, "%d", j);
      data = xbt_dict_get(head, key);
      xbt_test_assert2(!strcmp(key, (char *) data),
                       "with get, key=%s != data=%s", key, (char *) data);
      data = xbt_dict_get_ext(head, key, strlen(key));
      xbt_test_assert2(!strcmp(key, (char *) data),
                       "with get_ext, key=%s != data=%s", key, (char *) data);
    }
  }
  free(key);

  xbt_test_add1("Remove my %d elements", NB_ELM);
  key = xbt_malloc(10);
  for (j = 0; j < NB_ELM; j++) {
    /* if (!(j%10000)) printf("."); fflush(stdout); */

    sprintf(key, "%d", j);
    xbt_dict_remove(head, key);
  }
  free(key);


  xbt_test_add0("Free the structure (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

static void str_free(void *s)
{
  char *c = *(char **) s;
  free(c);
}

XBT_TEST_UNIT("multicrash", test_dict_multicrash, "Multi-dict crash test")
{

#undef NB_ELM
#define NB_ELM 100              /*00 */
#define DEPTH 5
#define KEY_SIZE 512
#define NB_TEST 20              /*20 */
  int verbose = 0;

  xbt_dict_t mdict = NULL;
  int i, j, k, l;
  xbt_dynar_t keys = xbt_dynar_new(sizeof(char *), str_free);
  void *data;
  char *key;


  xbt_test_add0("Generic multicache CRASH test");
  xbt_test_log4(" Fill the struct and frees it %d times, using %d elements, "
                "depth of multicache=%d, key size=%d",
                NB_TEST, NB_ELM, DEPTH, KEY_SIZE);

  for (l = 0; l < DEPTH; l++) {
    key = xbt_malloc(KEY_SIZE);
    xbt_dynar_push(keys, &key);
  }

  for (i = 0; i < NB_TEST; i++) {
    mdict = xbt_dict_new();
    VERB1("mdict=%p", mdict);
    if (verbose > 0)
      printf("Test %d\n", i);
    /* else if (i%10) printf("."); else printf("%d",i/10); */

    for (j = 0; j < NB_ELM; j++) {
      if (verbose > 0)
        printf("  Add {");

      for (l = 0; l < DEPTH; l++) {
        key = *(char **) xbt_dynar_get_ptr(keys, l);

        for (k = 0; k < KEY_SIZE - 1; k++)
          key[k] = rand() % ('z' - 'a') + 'a';

        key[k] = '\0';

        if (verbose > 0)
          printf("%p=%s %s ", key, key, (l < DEPTH - 1 ? ";" : "}"));
      }
      if (verbose > 0)
        printf("in multitree %p.\n", mdict);

      xbt_multidict_set(mdict, keys, xbt_strdup(key), free);

      data = xbt_multidict_get(mdict, keys);

      xbt_test_assert2(data && !strcmp((char *) data, key),
                       "Retrieved value (%s) does not match the given one (%s)\n",
                       (char *) data, key);
    }
    xbt_dict_free(&mdict);
  }

  xbt_dynar_free(&keys);

  /*  if (verbose>0)
     xbt_dict_dump(mdict,&xbt_dict_print); */

  xbt_dict_free(&mdict);
  xbt_dynar_free(&keys);

}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

