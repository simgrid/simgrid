/* file_appender - a dumb log appender which simply prints to stdout        */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log_private.h"
#include <stdio.h>

/**
 * The root category's default logging function.
 */

extern const char *xbt_log_priority_names[8];


static void append_file(xbt_log_appender_t this_appender, char *str)
{
  fprintf((FILE *) (this_appender->data), "%s", str);
}

static void free_(xbt_log_appender_t this_)
{
  if (this_->data != stderr)
    fclose(this_->data);
}



xbt_log_appender_t xbt_log_appender_file_new(char *arg)
{
  xbt_log_appender_t res = xbt_new0(s_xbt_log_appender_t, 1);
  res->do_append = append_file;
  res->free_ = free_;
  if (arg)
    res->data = (void *) fopen(arg, "w");
  else
    res->data = (void *) stderr;
  return res;
}
