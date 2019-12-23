/* dict - a generic dictionary, variation over hash table                   */

/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"

#include "simgrid/Exception.hpp"
#include <cstdio>
#include <cstring>
#include <random>

#include "catch.hpp"

#define STR(str) ((str) ? (str) : "(null)")

static constexpr int NB_ELM    = 20000;
static constexpr int SIZEOFKEY = 1024;

static void debugged_add_ext(xbt_dict_t head, const char* key, const char* data_to_fill)
{
  char* data = xbt_strdup(data_to_fill);

  INFO("Add " << STR(data_to_fill) << " under " << STR(key));

  xbt_dict_set(head, key, data);
}

static void debugged_add(xbt_dict_t head, const char* key)
{
  debugged_add_ext(head, key, key);
}

static xbt_dict_t new_fixture()
{
  INFO("Fill in the dictionary");

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

static void search_ext(xbt_dict_t head, const char* key, const char* data)
{
  INFO("Search " << STR(key));
  char* found = (char*)xbt_dict_get(head, key);
  INFO("Found " << STR(found));
  if (data) {
    REQUIRE(found); // data do not match expectations: found null while searching for data
    if (found)
      REQUIRE(not strcmp(data, found)); // data do not match expectations: found another string while searching for data
  } else {
    REQUIRE(not found); // data do not match expectations: found something while searching for null
  }
}

static void search(xbt_dict_t head, const char* key)
{
  search_ext(head, key, key);
}

static void debugged_remove(xbt_dict_t head, const char* key)
{
  INFO("Remove '" << STR(key) << "'");
  xbt_dict_remove(head, key);
}

static void traverse(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* data;
  int i = 0;

  xbt_dict_foreach (head, cursor, key, data) {
    INFO("Seen #" << ++i << ": " << STR(key) << "->" << STR(data));
    REQUIRE((key && data && strcmp(key, data) == 0)); //  key != value
  }
}

static void search_not_found(xbt_dict_t head, const char* data)
{
  INFO("Search " << STR(data) << " (expected not to be found)");
  REQUIRE_THROWS_AS(xbt_dict_get(head, data), std::out_of_range);
}

static void count(xbt_dict_t dict, int length)
{
  INFO("Count elements (expecting " << length << ")");
  REQUIRE(xbt_dict_length(dict) == length); // Announced length differs

  xbt_dict_cursor_t cursor;
  char* key;
  void* data;
  int effective = 0;
  xbt_dict_foreach (dict, cursor, key, data)
    effective++;

  REQUIRE(effective == length); // Effective length differs
}

static void count_check_get_key(xbt_dict_t dict, int length)
{
  xbt_dict_cursor_t cursor;
  char* key;
  void* data;
  int effective = 0;

  INFO("Count elements (expecting " << length << "), and test the getkey function");
  REQUIRE(xbt_dict_length(dict) == length); // Announced length differs

  xbt_dict_foreach (dict, cursor, key, data) {
    effective++;
    char* key2 = xbt_dict_get_key(dict, data);
    xbt_assert(not strcmp(key, key2), "The data was registered under %s instead of %s as expected", key2, key);
  }

  REQUIRE(effective == length); // Effective length differs
}

static int countelems(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor;
  char* key;
  void* data;
  int res = 0;

  xbt_dict_foreach (head, cursor, key, data) {
    res++;
  }
  return res;
}

TEST_CASE("xbt::dict: dict data container", "dict")
{
  SECTION("Basic usage: change, retrieve and traverse homogeneous dicts")
  {
    INFO("Traversal the null dictionary");
    traverse(nullptr);

    INFO("Traversal and search the empty dictionary");
    xbt_dict_t head = xbt_dict_new_homogeneous(&free);
    traverse(head);
    REQUIRE_THROWS_AS(debugged_remove(head, "12346"), std::out_of_range);
    xbt_dict_free(&head);

    INFO("Traverse the full dictionary");
    head = new_fixture();
    count_check_get_key(head, 7);

    debugged_add_ext(head, "toto", "tutu");
    search_ext(head, "toto", "tutu");
    debugged_remove(head, "toto");

    search(head, "12a");
    traverse(head);

    INFO("Free the dictionary (twice)");
    xbt_dict_free(&head);
    xbt_dict_free(&head);

    /* CHANGING */
    head = new_fixture();
    count_check_get_key(head, 7);
    INFO("Change 123 to 'Changed 123'");
    xbt_dict_set(head, "123", xbt_strdup("Changed 123"));
    count_check_get_key(head, 7);

    INFO("Change 123 back to '123'");
    xbt_dict_set(head, "123", xbt_strdup("123"));
    count_check_get_key(head, 7);

    INFO("Change 12a to 'Dummy 12a'");
    xbt_dict_set(head, "12a", xbt_strdup("Dummy 12a"));
    count_check_get_key(head, 7);

    INFO("Change 12a to '12a'");
    xbt_dict_set(head, "12a", xbt_strdup("12a"));
    count_check_get_key(head, 7);

    INFO("Traverse the resulting dictionary");
    traverse(head);

    /* RETRIEVE */
    INFO("Search 123");
    const char* data = (char*)xbt_dict_get(head, "123");
    REQUIRE((data && strcmp("123", data) == 0));

    search_not_found(head, "Can't be found");
    search_not_found(head, "123 Can't be found");
    search_not_found(head, "12345678 NOT");

    search(head, "12a");
    search(head, "12b");
    search(head, "12");
    search(head, "123456");
    search(head, "1234");
    search(head, "123457");

    INFO("Traverse the resulting dictionary");
    traverse(head);

    INFO("Free the dictionary twice");
    xbt_dict_free(&head);
    xbt_dict_free(&head);

    INFO("Traverse the resulting dictionary");
    traverse(head);
  }

  SECTION("Removing some values from homogeneous dicts")
  {
    xbt_dict_t head = new_fixture();
    count(head, 7);
    INFO("Remove non existing data");
    REQUIRE_THROWS_AS(debugged_remove(head, "Does not exist"), std::out_of_range);
    traverse(head);

    xbt_dict_free(&head);

    INFO("Remove each data manually (traversing the resulting dictionary each time)");
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
    REQUIRE_THROWS_AS(debugged_remove(head, "12346"), std::out_of_range);
    traverse(head);
    debugged_remove(head, "1234");
    traverse(head);
    debugged_remove(head, "123457");
    traverse(head);
    debugged_remove(head, "123");
    traverse(head);
    REQUIRE_THROWS_AS(debugged_remove(head, "12346"), std::out_of_range);
    traverse(head);

    INFO("Free dict, create new fresh one, and then reset the dict");
    xbt_dict_free(&head);
    head = new_fixture();
    xbt_dict_reset(head);
    count(head, 0);
    traverse(head);

    INFO("Free the dictionary twice");
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }

  SECTION("nullptr data management")
  {
    xbt_dict_t head = new_fixture();

    INFO("Store nullptr under 'null'");
    xbt_dict_set(head, "null", nullptr);
    search_ext(head, "null", nullptr);

    INFO("Check whether I see it while traversing...");
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    bool found = false;
    char* data;

    xbt_dict_foreach (head, cursor, key, data) {
      INFO("Seen: " << STR(key) << "->" << STR(data));
      if (key && strcmp(key, "null") == 0)
        found = true;
    }
    REQUIRE(found); // the key 'null', associated to nullptr is not found

    xbt_dict_free(&head);
  }

  SECTION("Crash test")
  {
    std::random_device rd;
    std::default_random_engine rnd_engine(rd());

    for (int i = 0; i < 10; i++) {
      INFO("CRASH test number " << i + 1 << " (" << 10 - i - 1 << " to go)");
      INFO("Fill the struct, count its elems and frees the structure");
      INFO("using 1000 elements with " << SIZEOFKEY << " chars long randomized keys.");
      xbt_dict_t head = xbt_dict_new_homogeneous(free);
      for (int j = 0; j < 1000; j++) {
        const char* data = nullptr;
        char* key  = (char*)xbt_malloc(SIZEOFKEY);

        do {
          for (int k = 0; k < SIZEOFKEY - 1; k++) {
            key[k] = rnd_engine() % ('z' - 'a') + 'a';
          }
          key[SIZEOFKEY - 1] = '\0';
          data               = (char*)xbt_dict_get_or_null(head, key);
        } while (data != nullptr);

        xbt_dict_set(head, key, key);
        data = (char*)xbt_dict_get(head, key);
        REQUIRE(not strcmp(key, data)); // Retrieved value != Injected value

        count(head, j + 1);
      }
      traverse(head);
      xbt_dict_free(&head);
      xbt_dict_free(&head);
    }

    xbt_dict_t head = xbt_dict_new_homogeneous(&free);
    INFO("Fill " << NB_ELM << " elements, with keys being the number of element");
    for (int j = 0; j < NB_ELM; j++) {
      char* key = (char*)xbt_malloc(10);

      snprintf(key, 10, "%d", j);
      xbt_dict_set(head, key, key);
    }

    INFO("Count the elements (retrieving the key and data for each)");
    INFO("There is " << countelems(head) << " elements");

    INFO("Search my " << NB_ELM << " elements 20 times");
    char* key = (char*)xbt_malloc(10);
    for (int i = 0; i < 20; i++) {
      for (int j = 0; j < NB_ELM; j++) {
        snprintf(key, 10, "%d", j);
        void* data = xbt_dict_get(head, key);
        REQUIRE(not strcmp(key, (char*)data)); // with get, key != data
        data = xbt_dict_get_ext(head, key, strlen(key));
        REQUIRE(not strcmp(key, (char*)data)); // with get_ext, key != data
      }
    }
    xbt_free(key);

    INFO("Remove my " << NB_ELM << " elements");
    key = (char*)xbt_malloc(10);
    for (int j = 0; j < NB_ELM; j++) {
      snprintf(key, 10, "%d", j);
      xbt_dict_remove(head, key);
    }
    xbt_free(key);

    INFO("Free the object (twice)");
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }

  SECTION("Test dictionary with int keys")
  {
    xbt_dict_t dict = xbt_dict_new_homogeneous(nullptr);
    int count       = 500;

    INFO("Insert elements");
    for (int i = 0; i < count; ++i)
      xbt_dict_set_ext(dict, (char*)&i, sizeof(i), (void*)(intptr_t)i);
    REQUIRE(xbt_dict_size(dict) == (unsigned)count); // Bad number of elements in the dictionary

    INFO("Check elements");
    for (int i = 0; i < count; ++i) {
      xbt_dict_get_ext(dict, (char*)&i, sizeof(i));
      REQUIRE(xbt_dict_size(dict) == (unsigned)count); // Unexpected value at index i
    }

    INFO("Free the array");
    xbt_dict_free(&dict);
  }
}
