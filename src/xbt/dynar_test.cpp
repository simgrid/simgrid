/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dynar.h"
#include "xbt/sysdep.h"

#include "catch.hpp"

constexpr int NB_ELEM = 5000;

TEST_CASE("xbt::dynar: generic C vector", "dynar")
{
  SECTION("Dynars of integers")
  {
    /* Vars_decl [doxygen cruft] */
    int cpt;
    unsigned int cursor;

    INFO("==== Traverse the empty dynar");
    xbt_dynar_t d = xbt_dynar_new(sizeof(int), nullptr);
    xbt_dynar_foreach (d, cursor, cpt) {
      xbt_die("Damnit, there is something in the empty dynar");
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << " int, set them again 3 times, traverse them, shift them");
    /* Populate_ints [doxygen cruft] */
    /* 1. Populate the dynar */
    d = xbt_dynar_new(sizeof(int), nullptr);
    for (int i = 0; i < NB_ELEM; i++) {
      xbt_dynar_push_as(d, int, i); /* This is faster (and possible only with scalars) */
      /* xbt_dynar_push(d, &i);      This would also work */
    }

    /* 2. Traverse manually the dynar */
    for (int i = 0; i < NB_ELEM; i++) {
      int* iptr = (int*)xbt_dynar_get_ptr(d, i);
      REQUIRE(i == *iptr); // The retrieved value is not the same than the injected one
    }

    /* 3. Traverse the dynar using the neat macro to that extend */
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cursor == (unsigned int)cpt); // The retrieved value is not the same than the injected one
    }
    /* end_of_traversal */

    for (int i = 0; i < NB_ELEM; i++)
      *(int*)xbt_dynar_get_ptr(d, i) = i;

    for (int i = 0; i < NB_ELEM; i++)
      *(int*)xbt_dynar_get_ptr(d, i) = i;

    for (int i = 0; i < NB_ELEM; i++)
      *(int*)xbt_dynar_get_ptr(d, i) = i;

    int count = 0;
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cpt == count); // The retrieved value is not the same than the injected one
      count++;
    }
    REQUIRE(count == NB_ELEM); // Cannot retrieve all my values. cpt is the last one I got

    /* shifting [doxygen cruft] */
    /* 4. Shift all the values */
    for (int i = 0; i < NB_ELEM; i++) {
      int val;
      xbt_dynar_shift(d, &val);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one
    }
    REQUIRE(xbt_dynar_is_empty(d));

    for (int i = 0; i < NB_ELEM; i++) {
      xbt_dynar_push_as(d, int, -1);
    }
    int* pi;
    xbt_dynar_foreach_ptr(d, cursor, pi) { *pi = 0; }
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cpt == 0); // The value is not the same as the expected one.
    }
    xbt_dynar_foreach_ptr(d, cursor, pi) { *pi = 1; }
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cpt == 1); // The value is not the same as the expected one
    }

    /* 5. Free the resources */
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Unshift/pop " << NB_ELEM << " int");
    d = xbt_dynar_new(sizeof(int), nullptr);
    for (int i = 0; i < NB_ELEM; i++) {
      xbt_dynar_unshift(d, &i);
    }
    for (int i = 0; i < NB_ELEM; i++) {
      int val = xbt_dynar_pop_as(d, int);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << "%d int, insert 1000 int in the middle, shift everything");
    d = xbt_dynar_new(sizeof(int), nullptr);
    for (int i = 0; i < NB_ELEM; i++) {
      xbt_dynar_push_as(d, int, i);
    }
    for (int i = 0; i < NB_ELEM / 5; i++) {
      xbt_dynar_insert_at_as(d, NB_ELEM / 2, int, i);
    }

    for (int i = 0; i < NB_ELEM / 2; i++) {
      int val;
      xbt_dynar_shift(d, &val);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one at the beginning
    }
    for (int i = 999; i >= 0; i--) {
      int val;
      xbt_dynar_shift(d, &val);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one in the middle
    }
    for (int i = 2500; i < NB_ELEM; i++) {
      int val;
      xbt_dynar_shift(d, &val);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one at the end
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << " int, remove 2000-4000. free the rest");
    d = xbt_dynar_new(sizeof(int), nullptr);
    for (int i = 0; i < NB_ELEM; i++)
      xbt_dynar_push_as(d, int, i);

    for (int i = 2000; i < 4000; i++) {
      int val;
      xbt_dynar_remove_at(d, 2000, &val);
      REQUIRE(val == i); // Remove a bad value
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */
  }

  /*******************************************************************************/
  SECTION("Using the xbt_dynar_insert and xbt_dynar_remove functions")
  {
    xbt_dynar_t d = xbt_dynar_new(sizeof(unsigned int), nullptr);
    unsigned int cursor;

    INFO("==== Insert " << NB_ELEM << " int, traverse them, remove them");
    /* Populate_ints [doxygen cruft] */
    /* 1. Populate the dynar */
    for (int i = 0; i < NB_ELEM; i++) {
      xbt_dynar_insert_at(d, i, &i);
    }

    /* 3. Traverse the dynar */
    int cpt;
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cursor == (unsigned int)cpt); // The retrieved value is not the same than the injected one
    }
    /* end_of_traversal */

    /* Re-fill with the same values using set_as (and re-verify) */
    for (int i = 0; i < NB_ELEM; i++)
      xbt_dynar_set_as(d, i, int, i);
    xbt_dynar_foreach (d, cursor, cpt)
      REQUIRE(cursor == (unsigned int)cpt); // The retrieved value is not the same than the injected one

    for (int i = 0; i < NB_ELEM; i++) {
      int val;
      xbt_dynar_remove_at(d, 0, &val);
      REQUIRE(i == val); // The retrieved value is not the same than the injected one
    }
    REQUIRE(xbt_dynar_is_empty(d));
    xbt_dynar_free(&d);

    /* ********************* */
    INFO("==== Insert " << NB_ELEM << " int in reverse order, traverse them, remove them");
    d = xbt_dynar_new(sizeof(int), nullptr);
    for (int i = NB_ELEM - 1; i >= 0; i--) {
      xbt_dynar_replace(d, i, &i);
    }

    /* 3. Traverse the dynar */
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(cursor == (unsigned)cpt); // The retrieved value is not the same than the injected one
    }
    /* end_of_traversal */

    for (int i = NB_ELEM - 1; i >= 0; i--) {
      int val;
      xbt_dynar_remove_at(d, xbt_dynar_length(d) - 1, &val);
      REQUIRE(val == i); // The retrieved value is not the same than the injected one
    }
    REQUIRE(xbt_dynar_is_empty(d));
    xbt_dynar_free(&d);
  }

  /*******************************************************************************/
  SECTION("Dynars of doubles")
  {
    xbt_dynar_t d;
    int cpt;
    unsigned int cursor;
    double d1;
    double d2;

    INFO("==== Traverse the empty dynar");
    d = xbt_dynar_new(sizeof(int), nullptr);
    xbt_dynar_foreach (d, cursor, cpt) {
      REQUIRE(false); // Damnit, there is something in the empty dynar
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push/shift 5000 doubles");
    d = xbt_dynar_new(sizeof(double), nullptr);
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_push(d, &d1);
    }
    xbt_dynar_foreach (d, cursor, d2) {
      d1 = (double)cursor;
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one
    }
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_shift(d, &d2);
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Unshift/pop 5000 doubles");
    d = xbt_dynar_new(sizeof(double), nullptr);
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_unshift(d, &d1);
    }
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_pop(d, &d2);
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
    d = xbt_dynar_new(sizeof(double), nullptr);
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_push(d, &d1);
    }
    for (int i = 0; i < 1000; i++) {
      d1 = (double)i;
      xbt_dynar_insert_at(d, 2500, &d1);
    }

    for (int i = 0; i < 2500; i++) {
      d1 = (double)i;
      xbt_dynar_shift(d, &d2);
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one at the beginning
    }
    for (int i = 999; i >= 0; i--) {
      d1 = (double)i;
      xbt_dynar_shift(d, &d2);
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one in the middle
    }
    for (int i = 2500; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_shift(d, &d2);
      REQUIRE(d1 == d2); // The retrieved value is not the same than the injected one at the end
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push 5000 double, remove 2000-4000. free the rest");
    d = xbt_dynar_new(sizeof(double), nullptr);
    for (int i = 0; i < 5000; i++) {
      d1 = (double)i;
      xbt_dynar_push(d, &d1);
    }
    for (int i = 2000; i < 4000; i++) {
      d1 = (double)i;
      xbt_dynar_remove_at(d, 2000, &d2);
      REQUIRE(d1 == d2); // Remove a bad value
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */
  }

  /*******************************************************************************/
  SECTION("Dynars of strings")
  {
    unsigned int iter;
    char buf[1024];
    char* s1;
    char* s2;

    INFO("==== Traverse the empty dynar");
    xbt_dynar_t d = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
    xbt_dynar_foreach (d, iter, s1) {
      REQUIRE(false); // Damnit, there is something in the empty dynar"
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << " strings, set them again 3 times, shift them");
    /* Populate_str [doxygen cruft] */
    d = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
    /* 1. Populate the dynar */
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      s1 = xbt_strdup(buf);
      xbt_dynar_push(d, &s1);
    }
    for (int k = 0; k < 3; k++) {
      for (int i = 0; i < NB_ELEM; i++) {
        snprintf(buf, 1023, "%d", i);
        s1 = xbt_strdup(buf);
        xbt_dynar_replace(d, i, &s1);
      }
    }
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_shift(d, &s2);
      REQUIRE(not strcmp(buf, s2)); // The retrieved value is not the same than the injected one
      xbt_free(s2);
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Unshift, traverse and pop " << NB_ELEM << " strings");
    d = xbt_dynar_new(sizeof(char**), &xbt_free_ref);
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      s1 = xbt_strdup(buf);
      xbt_dynar_unshift(d, &s1);
    }
    /* 2. Traverse the dynar with the macro */
    xbt_dynar_foreach (d, iter, s1) {
      snprintf(buf, 1023, "%u", NB_ELEM - iter - 1);
      REQUIRE(not strcmp(buf, s1)); // The retrieved value is not the same than the injected one
    }
    /* 3. Traverse the dynar with the macro */
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_pop(d, &s2);
      REQUIRE(not strcmp(buf, s2)); // The retrieved value is not the same than the injected one
      xbt_free(s2);
    }
    /* 4. Free the resources */
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << " strings, insert " << (NB_ELEM / 5) << " strings in the middle, shift everything");
    d = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      s1 = xbt_strdup(buf);
      xbt_dynar_push(d, &s1);
    }
    for (int i = 0; i < NB_ELEM / 5; i++) {
      snprintf(buf, 1023, "%d", i);
      s1 = xbt_strdup(buf);
      xbt_dynar_insert_at(d, NB_ELEM / 2, &s1);
    }

    for (int i = 0; i < NB_ELEM / 2; i++) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_shift(d, &s2);
      REQUIRE(not strcmp(buf, s2)); // The retrieved value is not the same than the injected one at the beginning
      xbt_free(s2);
    }
    for (int i = (NB_ELEM / 5) - 1; i >= 0; i--) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_shift(d, &s2);
      REQUIRE(not strcmp(buf, s2)); // The retrieved value is not the same than the injected one in the middle
      xbt_free(s2);
    }
    for (int i = NB_ELEM / 2; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_shift(d, &s2);
      REQUIRE(not strcmp(buf, s2)); // The retrieved value is not the same than the injected one at the end
      xbt_free(s2);
    }
    xbt_dynar_free(&d); /* This code is used both as example and as regression test, so we try to */
    xbt_dynar_free(&d); /* free the struct twice here to check that it's ok, but freeing  it only once */
    /* in your code is naturally the way to go outside a regression test */

    INFO("==== Push " << NB_ELEM << " strings, remove " << (2 * NB_ELEM / 5) << "-" << (4 * NB_ELEM / 5)
                      << ". free the rest");
    d = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
    for (int i = 0; i < NB_ELEM; i++) {
      snprintf(buf, 1023, "%d", i);
      s1 = xbt_strdup(buf);
      xbt_dynar_push(d, &s1);
    }
    for (int i = 2 * (NB_ELEM / 5); i < 4 * (NB_ELEM / 5); i++) {
      snprintf(buf, 1023, "%d", i);
      xbt_dynar_remove_at(d, 2 * (NB_ELEM / 5), &s2);
      REQUIRE(not strcmp(buf, s2)); // Remove a bad value
      xbt_free(s2);
    }
    xbt_dynar_free(&d); /* end_of_doxygen */
  }
}
