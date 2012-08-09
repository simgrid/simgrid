/* file_appender - a dumb log appender which simply prints to a file        */

/* Copyright (c) 2007-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log_private.h"
#include "smpi/private.h" // to access bench_begin/end. Not ultraclean, I confess
#include <stdio.h>

static void append_file(xbt_log_appender_t this_, char *str) {
  fputs(str, (FILE *) this_->data);
}

static void smpi_append_file(xbt_log_appender_t this_, char *str) {
  smpi_bench_end();
  fputs(str, (FILE *) this_->data);
  smpi_bench_begin();
}

static void free_(xbt_log_appender_t this_) {
  if (this_->data != stderr)
    fclose(this_->data);
}

void __smpi_bench_dont (void); // Stupid prototype
void __smpi_bench_dont (void) { /* I'm only a place-holder in case we link without SMPI */; }
void smpi_bench_begin(void) __attribute__ ((weak, alias ("__smpi_bench_dont")));
void smpi_bench_end(void)   __attribute__ ((weak, alias ("__smpi_bench_dont")));


XBT_LOG_EXTERNAL_CATEGORY(smpi); // To detect if SMPI is inited

xbt_log_appender_t xbt_log_appender_file_new(char *arg) {

  xbt_log_appender_t res = xbt_new0(s_xbt_log_appender_t, 1);
  if (_XBT_LOGV(smpi).initialized) // HACK to detect if we run in SMPI mode. Relies on MAIN__ source disposition
    res->do_append = smpi_append_file;
  else
    res->do_append = append_file;
  res->free_ = free_;
  if (arg)
    res->data = (void *) fopen(arg, "w");
  else
    res->data = (void *) stderr;
  return res;
}
