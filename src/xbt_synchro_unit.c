/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

# 58 "/home/navarrop/Developments/simgrid/src/xbt/xbt_synchro.c" 
#define NB_ELEM 50
#include "xbt/synchro.h"

XBT_LOG_EXTERNAL_CATEGORY(xbt_dyn);
XBT_LOG_DEFAULT_CATEGORY(xbt_dyn);

static void add100(int rank,void *data) {
  //INFO2("Thread%d: Add 100 to %d",rank,*(int*)data);
  *(int*)data +=100;
}

XBT_TEST_UNIT("dopar", test_dynar_dopar, "do parallel on dynars of integers")
{
  xbt_dynar_t d;
  int i, cpt;
  unsigned int cursor;

  xbt_test_add1("==== Push %d int, add 100 to each of them in parallel and check the results", NB_ELEM);
  d = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < NB_ELEM; cpt++) {
    xbt_dynar_push_as(d, int, cpt);     /* This is faster (and possible only with scalars) */
    xbt_test_log2("Push %d, length=%lu", cpt, xbt_dynar_length(d));
  }
  xbt_dynar_dopar(d,add100);
  cpt = 100;
  xbt_dynar_foreach(d, cursor, i) {
    xbt_test_assert2(i == cpt,
                     "The retrieved value is not the expected one (%d!=%d)",
                     i, cpt);
    cpt++;
  }
  xbt_dynar_free(&d);
}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

