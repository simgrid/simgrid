/* Copyright (c) 2009 Da SimGrid Team.  All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This datatype stores a a trace as produced by examples/simdag/dax, or other means
 * It can be replayed in simulation with examples/msg/actions or on a real platform with
 * examples/gras/replay.
 */

#ifndef XBT_WORKLOAD_H_
#define XBT_WORKLOAD_H_

#include "xbt/misc.h"
#include "xbt/dynar.h"

/* kind of elements */
#define XBT_WORKLOAD_COMPUTE 0
#define XBT_WORKLOAD_SEND 1
#define XBT_WORKLOAD_RECV 2

/* one command to do */
typedef struct {
  /* keep it in sync with function xbt_workload_declare_datadesc() */

  char *who;      /* the slave who should do it */
  char *comment;  /* a comment placed at the end of the line, if any */
  int action;     /* 0: compute(darg flops); 1: send darg bytes to strarg; 2: recv darg bytes from strarg */
  double date;    /* when it occured when the trace was captured */
  double d_arg;   /* double argument, if any */
  char * str_arg; /* string argument, if any */
} s_xbt_workload_elm_t, *xbt_workload_elm_t;

XBT_PUBLIC(xbt_workload_elm_t) xbt_workload_elm_parse(char *line);
XBT_PUBLIC(void) xbt_workload_elm_free(xbt_workload_elm_t cmd);
XBT_PUBLIC(void) xbt_workload_elm_free_voidp(void*cmd);
XBT_PUBLIC(char *)xbt_workload_elm_to_string(xbt_workload_elm_t cmd);
XBT_PUBLIC(int) xbt_workload_elm_cmp_who_date(const void* _c1, const void* _c2);
XBT_PUBLIC(void) xbt_workload_sort_who_date(xbt_dynar_t c);
XBT_PUBLIC(xbt_dynar_t) xbt_workload_parse_file(char *filename);

XBT_PUBLIC(void) xbt_workload_declare_datadesc(void);


typedef struct {
  int size;
  char *chunk;
} s_xbt_workload_data_chunk_t,*xbt_workload_data_chunk_t;
XBT_PUBLIC(xbt_workload_data_chunk_t) xbt_workload_data_chunk_new(int size);
XBT_PUBLIC(void) xbt_workload_data_chunk_free(xbt_workload_data_chunk_t c);

#endif /* XBT_WORKLOAD_H_ */
