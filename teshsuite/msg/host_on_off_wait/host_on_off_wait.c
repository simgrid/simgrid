/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int master(int argc, char *argv[])
{
  xbt_ex_t e;
  TRY {
    msg_host_t jupiter = MSG_host_by_name("Jupiter");
    XBT_INFO("Master waiting");
    MSG_process_sleep(1);

    XBT_INFO("Turning off the slave host");
    MSG_host_off(jupiter);
    XBT_INFO("Master has finished");
  }
  CATCH(e) {
    xbt_die("Exception caught in the master");
    return 1;
  }
  return 0;
}

static int slave(int argc, char *argv[])
{
  xbt_ex_t e;
  TRY {
    XBT_INFO("Slave waiting");
    // TODO, This should really be MSG_HOST_FAILURE
    MSG_process_sleep(5);
    XBT_ERROR("Slave should be off already.");
    return 1;
  }
  CATCH(e) {
    XBT_ERROR("Exception caught in the slave");
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("master", master, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("slave", slave, NULL, MSG_get_host_by_name("Jupiter"));

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
