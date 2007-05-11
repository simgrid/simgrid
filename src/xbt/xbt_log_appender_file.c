/* $Id$ */

/* file_appender - a dumb log appender which simply prints to stdout        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include <stdio.h>

/**
 * The root category's default logging function.
 */

extern const char *xbt_log_priority_names[7];


static void append_file(xbt_log_appender_t this_appender,
			char *str) {

  fprintf((FILE*)(this_appender->data), str);
}

xbt_log_appender_t xbt_log_appender_file_new(xbt_log_layout_t lout){
  xbt_log_appender_t res = xbt_new0(s_xbt_log_appender_t,1);
  res->layout = lout;
  res->do_append = append_file;
  res->data = (void*)stderr;
  return res;
}
