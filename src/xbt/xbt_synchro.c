/* xbt_synchro -- advanced multithreaded features                           */
/* Working on top of real threads in RL and of simulated processes in SG    */

/* Copyright 2009 Da SimGrid Team. All right reserved.                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "xbt/synchro.h"

typedef struct {
  xbt_dynar_t data;
  int rank;
  void_f_int_pvoid_t function;
  xbt_thread_t worker;
} s_worker_data_t,*worker_data_t;

static void worker_wait_n_free(void*w) {
  worker_data_t worker=*(worker_data_t*)w;
  xbt_thread_join(worker->worker);
  xbt_free(worker);
}
static void worker_wrapper(void *w) {
  worker_data_t me=(worker_data_t)w;
  (*me->function)(me->rank,xbt_dynar_get_ptr(me->data,me->rank));
  xbt_thread_exit();
}

void xbt_dynar_dopar(xbt_dynar_t datas, void_f_int_pvoid_t function) {
  xbt_dynar_t workers = xbt_dynar_new(sizeof(worker_data_t),worker_wait_n_free);
  unsigned int cursor;
  void *data;
  /* Start all workers */
  xbt_dynar_foreach(datas,cursor,data){
    worker_data_t w = xbt_new0(s_worker_data_t,1);
    w->data = datas;
    w->function = function;
    w->rank=cursor;
    xbt_dynar_push(workers,&w);
    w->worker = xbt_thread_create("dopar worker",worker_wrapper,w);
  }
  /* wait them all */
  xbt_dynar_free(&workers);
}

#ifdef SIMGRID_TEST
#define NB_ELEM 50
#include "xbt/synchro.h"

XBT_TEST_SUITE("synchro", "Advanced synchronization mecanisms");
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

#endif /* SIMGRID_TEST */


