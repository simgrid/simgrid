/* Copyright (c) 2007-2011, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef LOG_PRIVATE_H
#define LOG_PRIVATE_H

#include "xbt/log.h"
struct xbt_log_appender_s {
  void (*do_append) (xbt_log_appender_t this_appender, char *event);
  void (*free_) (xbt_log_appender_t this_);
  void *data;
};

struct xbt_log_layout_s {
  int (*do_layout) (xbt_log_layout_t l, xbt_log_event_t event, const char *fmt);
  void (*free_) (xbt_log_layout_t l);
  void *data;
};

/**
 * \ingroup XBT_log_implem
 * \param cat the category (not only its name, but the variable)
 * \param parent the parent cat
 *
 * Programatically alter a category's parent (don't use).
 */
XBT_PUBLIC(void) xbt_log_parent_set(xbt_log_category_t cat, xbt_log_category_t parent);


#endif                          /* LOG_PRIVATE_H */
