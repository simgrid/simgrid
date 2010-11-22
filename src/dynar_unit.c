/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 765 "xbt/dynar.c" 

#define NB_ELEM 5000

XBT_LOG_EXTERNAL_CATEGORY(xbt_dyn);
XBT_LOG_DEFAULT_CATEGORY(xbt_dyn);

XBT_TEST_UNIT("int", test_dynar_int, "Dynars of integers")
{
  /* Vars_decl [doxygen cruft] */
  xbt_dynar_t d;
  int i, cpt;
  unsigned int cursor;
  int *iptr;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, i) {
    xbt_assert0(0, "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1
      ("==== Push %d int, set them again 3 times, traverse them, shift them",
       NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);     /* This is faster (and possible only with scalars) */
    /* xbt_dynar_push(d,&cpt);       This would also work */
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 2. Traverse manually the dynar */
  for (cursor = 0; cursor < NB_ELEM; cursor++) {
    iptr = xbt_dynar_get_ptr(d, cursor);
    xbt_test_assert2(cursor == *iptr,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }

  /* 3. Traverse the dynar using the neat macro to that extend */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;
  /*     xbt_dynar_set(d,cpt,&cpt); */

  for (cpt = 0; cpt < NB_ELEM; cpt++)
    *(int *) xbt_dynar_get_ptr(d, cpt) = cpt;

  cpt = 0;
  xbt_dynar_foreach(d, cursor, i) {
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    cpt++;
  }
  xbt_test_assert2(cpt == NB_ELEM,
                   "Cannot retrieve my %d values. Last got one is %d",
                   NB_ELEM, cpt);

  /* shifting [doxygen cruft] */
  /* 4. Shift all the values */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    xbt_test_log2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 5. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Unshift/pop %d int", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_unshift(d, &cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    i = xbt_dynar_pop_as(d, int);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     i, cpt);
    xbt_test_log2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add1
      ("==== Push %d int, insert 1000 int in the middle, shift everything",
       NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 0; cpt < NB_ELEM/5; cpt++) {
    xbt_dynar_insert_at_as(d, NB_ELEM/2, int, cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  for (cpt = 0; cpt < NB_ELEM/2; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
                     i, cpt);
    DEBUG2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
                     i, cpt);
  }
  for (cpt = 2500; cpt < NB_ELEM; cpt++) {
    xbt_dynar_shift(d, &i);
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the same than the injected one at the end (%d!=%d)",
                     i, cpt);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Push %d int, remove 2000-4000. free the rest",
                NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_push_as(d, int, cpt);

  for (cpt = 2000; cpt < 4000; cpt++) {
    xbt_dynar_remove_at(d, 2000, &i);
    xbt_test_assert2(i == cpt,
                     "Remove a bad value. Got %d, expected %d", i, cpt);
    DEBUG2("remove %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("insert",test_dynar_insert,"Using the xbt_dynar_insert and xbt_dynar_remove functions")
{
  xbt_dynar_t d = xbt_dynar_new(sizeof(int), NULL);
  unsigned int cursor;
  int cpt;

  xbt_test_add1("==== Insert %d int, traverse them, remove them",NB_ELEM);
  /* Populate_ints [doxygen cruft] */
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_insert_at(d, cpt, &cpt);
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  /* Re-fill with the same values using set_as (and re-verify) */
  for (cpt = 0; cpt < NB_ELEM; cpt++)
    xbt_dynar_set_as(d, cpt, int, cpt);
  xbt_dynar_foreach(d, cursor, cpt)
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);

  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    int val;
    xbt_dynar_remove_at(d,0,&val);
    xbt_test_assert2(cpt == val,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  xbt_test_assert1(xbt_dynar_length(d) == 0,
                   "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);

  /* ********************* */
  xbt_test_add1("==== Insert %d int in reverse order, traverse them, remove them",NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = NB_ELEM-1; cpt >=0; cpt--) {
    xbt_dynar_replace(d, cpt, &cpt);
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }

  /* 3. Traverse the dynar */
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert2(cursor == cpt,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  /* end_of_traversal */

  for (cpt =NB_ELEM-1; cpt >=0; cpt--) {
    int val;
    xbt_dynar_remove_at(d,xbt_dynar_length(d)-1,&val);
    xbt_test_assert2(cpt == val,
                     "The retrieved value is not the same than the injected one (%d!=%d)",
                     cursor, cpt);
  }
  xbt_test_assert1(xbt_dynar_length(d) == 0,
                   "There is still %lu elements in the dynar after removing everything",
                   xbt_dynar_length(d));
  xbt_dynar_free(&d);
}

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("double", test_dynar_double, "Dynars of doubles")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int cursor;
  double d1, d2;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(d, cursor, cpt) {
    xbt_test_assert0(FALSE,
                     "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add0("==== Push/shift 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  xbt_dynar_foreach(d, cursor, d2) {
    d1 = (double) cursor;
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add0("==== Unshift/pop 5000 doubles");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_unshift(d, &d1);
  }
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_pop(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */



  xbt_test_add0
      ("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  for (cpt = 0; cpt < 1000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_insert_at(d, 2500, &d1);
  }

  for (cpt = 0; cpt < 2500; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one at the begining (%f!=%f)",
                     d1, d2);
    DEBUG2("Pop %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  for (cpt = 999; cpt >= 0; cpt--) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one in the middle (%f!=%f)",
                     d1, d2);
  }
  for (cpt = 2500; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_shift(d, &d2);
    xbt_test_assert2(d1 == d2,
                     "The retrieved value is not the same than the injected one at the end (%f!=%f)",
                     d1, d2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add0("==== Push 5000 double, remove 2000-4000. free the rest");
  d = xbt_dynar_new(sizeof(double), NULL);
  for (cpt = 0; cpt < 5000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_push(d, &d1);
  }
  for (cpt = 2000; cpt < 4000; cpt++) {
    d1 = (double) cpt;
    xbt_dynar_remove_at(d, 2000, &d2);
    xbt_test_assert2(d1 == d2,
                     "Remove a bad value. Got %f, expected %f", d2, d1);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */
}


/* doxygen_string_cruft */

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
XBT_TEST_UNIT("string", test_dynar_string, "Dynars of strings")
{
  xbt_dynar_t d;
  int cpt;
  unsigned int iter;
  char buf[1024];
  char *s1, *s2;

  xbt_test_add0("==== Traverse the empty dynar");
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  xbt_dynar_foreach(d, iter, s1) {
    xbt_test_assert0(FALSE,
                     "Damnit, there is something in the empty dynar");
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Push %d strings, set them again 3 times, shift them",
                NB_ELEM);
  /* Populate_str [doxygen cruft] */
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  /* 1. Populate the dynar */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_replace(d, cpt, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */

  xbt_test_add1("==== Unshift, traverse and pop %d strings", NB_ELEM);
  d = xbt_dynar_new(sizeof(char **), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_unshift(d, &s1);
  }
  /* 2. Traverse the dynar with the macro */
  xbt_dynar_foreach(d, iter, s1) {
    sprintf(buf, "%d", NB_ELEM - iter - 1);
    xbt_test_assert2(!strcmp(buf, s1),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s1);
  }
  /* 3. Traverse the dynar with the macro */
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_pop(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  /* 4. Free the resources */
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add2
      ("==== Push %d strings, insert %d strings in the middle, shift everything",
       NB_ELEM, NB_ELEM / 5);
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 0; cpt < NB_ELEM / 5; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_insert_at(d, NB_ELEM / 2, &s1);
  }

  for (cpt = 0; cpt < NB_ELEM / 2; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one at the begining (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  for (cpt = (NB_ELEM / 5) - 1; cpt >= 0; cpt--) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one in the middle (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  for (cpt = NB_ELEM / 2; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_shift(d, &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "The retrieved value is not the same than the injected one at the end (%s!=%s)",
                     buf, s2);
    free(s2);
  }
  xbt_dynar_free(&d);           /* This code is used both as example and as regression test, so we try to */
  xbt_dynar_free(&d);           /* free the struct twice here to check that it's ok, but freeing  it only once */
  /* in your code is naturally the way to go outside a regression test */


  xbt_test_add3("==== Push %d strings, remove %d-%d. free the rest",
                NB_ELEM, 2 * (NB_ELEM / 5), 4 * (NB_ELEM / 5));
  d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(d, &s1);
  }
  for (cpt = 2 * (NB_ELEM / 5); cpt < 4 * (NB_ELEM / 5); cpt++) {
    sprintf(buf, "%d", cpt);
    xbt_dynar_remove_at(d, 2 * (NB_ELEM / 5), &s2);
    xbt_test_assert2(!strcmp(buf, s2),
                     "Remove a bad value. Got %s, expected %s", s2, buf);
    free(s2);
  }
  xbt_dynar_free(&d);           /* end_of_doxygen */
}


/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
#include "xbt/synchro.h"
static void pusher_f(void *a)
{
  xbt_dynar_t d = (xbt_dynar_t) a;
  int i;
  for (i = 0; i < 500; i++) {
    xbt_dynar_push(d, &i);
  }
}

static void poper_f(void *a)
{
  xbt_dynar_t d = (xbt_dynar_t) a;
  int i;
  int data;
  xbt_ex_t e;

  for (i = 0; i < 500; i++) {
    TRY {
      xbt_dynar_pop(d, &data);
    }
    CATCH(e) {
      if (e.category == bound_error) {
        xbt_ex_free(e);
        i--;
      } else {
        RETHROW;
      }
    }
  }
}


XBT_TEST_UNIT("synchronized int", test_dynar_sync_int, "Synchronized dynars of integers")
{
  /* Vars_decl [doxygen cruft] */
  xbt_dynar_t d;
  xbt_thread_t pusher, poper;

  xbt_test_add0("==== Have a pusher and a popper on the dynar");
  d = xbt_dynar_new_sync(sizeof(int), NULL);
  pusher = xbt_thread_create("pusher", pusher_f, d, 0 /*not joinable */ );
  poper = xbt_thread_create("poper", poper_f, d, 0 /*not joinable */ );
  xbt_thread_join(pusher);
  xbt_thread_join(poper);
  xbt_dynar_free(&d);
}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

