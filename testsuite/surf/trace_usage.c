/* A few tests for the trace library                                       */

/* Copyright (c) 2004, 2005, 2006, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "surf/trace_mgr.h"
#include "surf/surf.h"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test,
                             "Messages specific for surf example");

void test(void);
void test(void)
{
  tmgr_history_t history = tmgr_history_new();
  tmgr_trace_t trace_A = tmgr_trace_new("trace_A.txt");
  tmgr_trace_t trace_B = tmgr_trace_new("trace_B.txt");
  double next_event_date = -1.0;
  double value = -1.0;
  char *resource = NULL;
  char *host_A = strdup("Host A");
  char *host_B = strdup("Host B");

  tmgr_history_add_trace(history, trace_A, 1.0, 2, host_A);
  tmgr_history_add_trace(history, trace_B, 0.0, 0, host_B);

  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    DEBUG1("%g" " : \n", next_event_date);
    while (tmgr_history_get_next_event_leq(history, next_event_date,
                                           &value, (void **) &resource)) {
      DEBUG2("\t %s : " "%g" "\n", resource, value);
    }
    if (next_event_date > 1000)
      break;
  }

  tmgr_history_free(history);
  free(host_B);
  free(host_A);
}

#ifdef __BORLANDC__
#pragma argsused
#endif


int main(int argc, char **argv)
{
  surf_init(&argc, argv);
  test();
  surf_exit();
  return 0;
}
