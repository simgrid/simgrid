/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simgrid/msg.h"        /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "iterator.h"
#include "messages.h"
#include "broadcaster.h"
#include "peer.h"

/** @addtogroup MSG_examples
 * 
 *  - <b>chainsend/chainsend.c: Chainsend implementation</b>.
 */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_chainsend,
                             "Messages specific for chainsend");

/*
 Data structures
 */

/* Initialization stuff */
msg_error_t test_all(const char *platform_file,
                     const char *application_file);


/** Test function */
msg_error_t test_all(const char *platform_file,
                     const char *application_file)
{

  msg_error_t res = MSG_OK;

  XBT_DEBUG("test_all");

  /*  Simulation setting */
  MSG_create_environment(platform_file);

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

  MSG_launch_application(application_file);

  res = MSG_main();

  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

#ifdef _MSC_VER
  unsigned int prev_exponent_format =
      _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  MSG_init(&argc, argv);

  res = test_all(argv[1], argv[2]);

  XBT_INFO("Total simulation time: %e", MSG_get_clock());

#ifdef _MSC_VER
  _set_output_format(prev_exponent_format);
#endif

  return res != MSG_OK;
}
