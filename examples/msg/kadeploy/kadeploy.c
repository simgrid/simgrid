/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * Copyright (c) 2012. Maximiliano Geier.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
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
 *  - <b>kadeploy/kadeploy.c: Kadeploy implementation</b>.
 */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kadeploy,
                             "Messages specific for kadeploy");

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



  XBT_INFO("test_all");

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

  /*if (argc <= 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file <model>\n",
              argv[0]);
    XBT_CRITICAL
        ("example: %s msg_platform.xml msg_deployment.xml KCCFLN05_Vegas\n",
         argv[0]);
    exit(1);
  }*/

  /* Options for the workstation/model:

     KCCFLN05              => for maxmin
     KCCFLN05_proportional => for proportional (Vegas)
     KCCFLN05_Vegas        => for TCP Vegas
     KCCFLN05_Reno         => for TCP Reno
   */
  //MSG_config("workstation/model", argv[3]);

  res = test_all(argv[1], argv[2]);

  XBT_INFO("Total simulation time: %le", MSG_get_clock());

  MSG_clean();

#ifdef _MSC_VER
  _set_output_format(prev_exponent_format);
#endif

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
