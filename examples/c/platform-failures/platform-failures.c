/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"

#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <stdio.h> /* snprintf */

#define FINALIZE 221297 /* a magic number to tell people to stop working */

XBT_LOG_NEW_DEFAULT_CATEGORY(platform_failures, "Messages specific for this example");

static void master(int argc, char* argv[])
{
  xbt_assert(argc == 5);
  long number_of_tasks  = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  long task_comm_size   = xbt_str_parse_int(argv[3], "Invalid communication size: %s");
  long workers_count    = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (int i = 0; i < number_of_tasks; i++) {
    char mailbox_name[256];
    snprintf(mailbox_name, 255, "worker-%ld", i % workers_count);
    sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);

    XBT_INFO("Send a message to %s", mailbox_name);
    double* payload = (double*)xbt_malloc(sizeof(double));
    *payload        = task_comp_size;
    sg_comm_t comm  = sg_mailbox_put_async(mailbox, payload, task_comm_size);

    switch (sg_comm_wait_for(comm, 10.0)) {
      case SG_OK:
        XBT_INFO("Send to %s completed", mailbox_name);
        break;
      case SG_ERROR_NETWORK:
        XBT_INFO("Mmh. The communication with '%s' failed. Nevermind. Let's keep going!", mailbox_name);
        free(payload);
        break;
      case SG_ERROR_TIMEOUT:
        XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox_name);
        free(payload);
        break;
      default:
        xbt_die("Unexpected behavior");
    }
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) {
    char mailbox_name[256];
    snprintf(mailbox_name, 255, "worker-%ld", i % workers_count);
    sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);
    double* payload      = (double*)xbt_malloc(sizeof(double));
    *payload             = FINALIZE;
    sg_comm_t comm       = sg_mailbox_put_async(mailbox, payload, 0);

    switch (sg_comm_wait_for(comm, 1.0)) {
      case SG_ERROR_NETWORK:
        XBT_INFO("Mmh. Can't reach '%s'! Nevermind. Let's keep going!", mailbox_name);
        free(payload);
        break;
      case SG_ERROR_TIMEOUT:
        XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox_name);
        free(payload);
        break;
      case SG_OK:
        /* nothing */
        break;
      default:
        xbt_die("Unexpected behavior with '%s'", mailbox_name);
    }
  }

  XBT_INFO("Goodbye now!");
}

static void worker(int argc, char* argv[])
{
  xbt_assert(argc == 2);
  char mailbox_name[80];
  long id = xbt_str_parse_int(argv[1], "Invalid argument %s");

  snprintf(mailbox_name, 79, "worker-%ld", id);
  sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);

  while (1) {
    XBT_INFO("Waiting a message on %s", mailbox_name);
    double* payload;
    sg_comm_t comm     = sg_mailbox_get_async(mailbox, (void**)&payload);
    sg_error_t retcode = sg_comm_wait(comm);
    if (retcode == SG_OK) {
      if (*payload == FINALIZE) {
        free(payload);
        break;
      } else {
        double comp_size = *payload;
        free(payload);
        XBT_INFO("Start execution...");
        sg_actor_execute(comp_size);
        XBT_INFO("Execution complete.");
      }
    } else if (retcode == SG_ERROR_NETWORK) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    }
  }
  XBT_INFO("I'm done. See you!");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  simgrid_register_function("master", master);
  simgrid_register_function("worker", worker);
  simgrid_load_deployment(argv[2]);

  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
