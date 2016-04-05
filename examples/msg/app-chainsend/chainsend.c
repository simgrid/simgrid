/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "iterator.h"
#include "messages.h"
#include "broadcaster.h"
#include "peer.h"

/** @addtogroup MSG_examples
 * 
 *  - <b>chainsend: MSG implementation of a file broadcasting system, similar to Kastafior (from Kadeploy).</b>.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_chainsend, "Messages specific for chainsend");

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  MSG_create_environment(argv[1]);

  /* Trace categories */
  TRACE_category_with_color("host0", "0 0 1");
  TRACE_category_with_color("host1", "0 1 0");
  TRACE_category_with_color("host2", "0 1 1");
  TRACE_category_with_color("host3", "1 0 0");
  TRACE_category_with_color("host4", "1 0 1");
  TRACE_category_with_color("host5", "0 0 0");
  TRACE_category_with_color("host6", "1 1 0");
  TRACE_category_with_color("host7", "1 1 1");
  TRACE_category_with_color("host8", "0 1 0");

  /*   Application deployment */
  MSG_function_register("broadcaster", broadcaster);
  MSG_function_register("peer", peer);

  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Total simulation time: %e", MSG_get_clock());

  return res != MSG_OK;
}
