/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(exec_async, "Messages specific for this example");

/* This actor simply waits for its task completion after starting it.
 * That's exactly equivalent to synchronous execution. */
static void waiter(int argc, char* argv[])
{
  double computation_amount = sg_host_speed(sg_host_self());
  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg_exec_t activity = sg_actor_exec_init(computation_amount);
  sg_exec_start(activity);
  sg_exec_wait(activity);

  XBT_INFO("Goodbye now!");
}

/* This actor tests the ongoing execution until its completion, and don't wait before it's terminated. */
static void monitor(int argc, char* argv[])
{
  double computation_amount = sg_host_speed(sg_host_self());
  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg_exec_t activity = sg_actor_exec_init(computation_amount);
  sg_exec_start(activity);

  while (!sg_exec_test(activity)) {
    XBT_INFO("Remaining amount of flops: %g (%.0f%%)", sg_exec_get_remaining(activity),
             100 * sg_exec_get_remaining_ratio(activity));
    sg_actor_sleep_for(0.3);
  }

  XBT_INFO("Goodbye now!");
}

/* This actor cancels the ongoing execution after a while. */
static void canceller(int argc, char* argv[])
{
  double computation_amount = sg_host_speed(sg_host_self());

  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg_exec_t activity = sg_actor_exec_init(computation_amount);
  sg_exec_start(activity);
  sg_actor_sleep_for(0.5);
  XBT_INFO("I changed my mind, cancel!");
  sg_exec_cancel(activity);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);

  sg_host_t fafard  = sg_host_by_name("Fafard");
  sg_host_t ginette = sg_host_by_name("Ginette");
  sg_host_t boivin  = sg_host_by_name("Boivin");

  sg_actor_create("wait", fafard, waiter, 0, NULL);
  sg_actor_create("monitor", ginette, monitor, 0, NULL);
  sg_actor_create("cancel", boivin, canceller, 0, NULL);

  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
