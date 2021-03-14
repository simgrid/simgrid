/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef LOG_PRIVATE_H
#define LOG_PRIVATE_H

#include "xbt/log.h"
#include <array>

struct xbt_log_appender_s {
  void (*do_append)(const s_xbt_log_appender_t* this_appender, const char* event);
  void (*free_)(const s_xbt_log_appender_t* this_);
  void *data;
};

struct xbt_log_layout_s {
  bool (*do_layout)(const s_xbt_log_layout_t* l, xbt_log_event_t event, const char* fmt);
  void (*free_)(const s_xbt_log_layout_t* l);
  void *data;
};

extern const std::array<const char*, xbt_log_priority_infinite> xbt_log_priority_names;

/**
 * @ingroup XBT_log_implem
 * @param cat the category (not only its name, but the variable)
 * @param parent the parent cat
 *
 * Programmatically alter a category's parent (don't use).
 */
XBT_PUBLIC void xbt_log_parent_set(xbt_log_category_t cat, xbt_log_category_t parent);

#endif
