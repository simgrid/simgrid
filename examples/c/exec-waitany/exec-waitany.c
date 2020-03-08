/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(exec_waitany, "Messages specific for this example");

static void worker(int argc, char* argv[])
{
  xbt_assert(argc > 1);
  int with_timeout = !strcmp(argv[1], "true");

  /* Vector in which we store all pending executions*/
  sg_exec_t* pending_execs = malloc(sizeof(sg_exec_t) * 3);
  int pending_execs_count  = 0;

  for (int i = 0; i < 3; i++) {
    char* name    = bprintf("Exec-%d", i);
    double amount = (6 * (i % 2) + i + 1) * sg_host_speed(sg_host_self());

    sg_exec_t exec = sg_actor_exec_init(amount);
    sg_exec_set_name(exec, name);
    pending_execs[pending_execs_count++] = exec;
    sg_exec_start(exec);

    XBT_INFO("Activity %s has started for %.0f seconds", name, amount / sg_host_speed(sg_host_self()));
    free(name);
  }

  /* Now that executions were initiated, wait for their completion, in order of termination.
   *
   * This loop waits for first terminating execution with wait_any() and remove it with erase(), until all execs are
   * terminated.
   */
  while (pending_execs_count > 0) {
    int pos;
    if (with_timeout)
      pos = sg_exec_wait_any_for(pending_execs, pending_execs_count, 4);
    else
      pos = sg_exec_wait_any(pending_execs, pending_execs_count);

    if (pos < 0) {
      XBT_INFO("Do not wait any longer for an activity");
      pending_execs_count = 0;
    } else {
      XBT_INFO("Activity at position %d is complete", pos);
      memmove(pending_execs + pos, pending_execs + pos + 1, sizeof(sg_exec_t) * (pending_execs_count - pos - 1));
      pending_execs_count--;
    }
    XBT_INFO("%d activities remain pending", pending_execs_count);
  }

  free(pending_execs);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);

  const char* worker_argv[] = {"worker", "false"};
  sg_actor_create("worker", sg_host_by_name("Tremblay"), worker, 2, worker_argv);

  worker_argv[1] = "true";
  sg_actor_create("worker_timeout", sg_host_by_name("Tremblay"), worker, 2, worker_argv);

  simgrid_run();
  return 0;
}
