/* log_usage - A test of normal usage of the log facilities                 */

/* Copyright (c) 2004-2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(test, top, "Logging specific to this test");
XBT_LOG_NEW_CATEGORY(top, "Useless test channel");

static void dolog(const char *settings)
{
  XBT_INFO("Test with the settings '%s'", settings);
  xbt_log_control_set(settings);
  XBT_DEBUG("val=%d", 1);
  XBT_WARN("val=%d", 2);
  XBT_CDEBUG(top, "val=%d%s", 3, "!");
  XBT_CRITICAL("false alarm%s%s%s%s%s%s", "", "", "", "", "", "!");
}

int main(int argc, char **argv)
{
  xbt_init(&argc, argv);

  dolog("");
  dolog(" ");
  dolog(" test.thres:info root.thres:info  ");
#ifndef NDEBUG
  dolog(" test.thres:debug ");
#endif
  dolog(" test.thres:verbose root.thres:error ");
  dolog(" test.thres:critical ");

  return 0;
}
