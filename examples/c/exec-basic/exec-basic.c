/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(exec_basic, "Messages specific for this example");

static void executor(int argc, char* argv[])
{
  /* sg_actor_execute() tells SimGrid to pause the calling actor
   * until its host has computed the amount of flops passed as a parameter */
  sg_actor_execute(98095);
  XBT_INFO("Done.");

  /* This simple example does not do anything beyond that */
}

static void privileged(int argc, char* argv[])
{
  /* sg_actor_execute_with_priority() specifies that this execution gets a larger share of the resource.
   *
   * Since the priority is 2, it computes twice as fast as a regular one.
   *
   * So instead of a half/half sharing between the two executions,
   * we get a 1/3 vs 2/3 sharing. */
  sg_actor_execute_with_priority(98095, 2);
  XBT_INFO("Done.");

  /* Note that the timings printed when executing this example are a bit misleading,
   * because the uneven sharing only last until the privileged actor ends.
   * After this point, the unprivileged one gets 100% of the CPU and finishes
   * quite quickly. */
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("executor", sg_host_by_name("Tremblay"), executor, 0, NULL);
  sg_actor_create("privileged", sg_host_by_name("Tremblay"), privileged, 0, NULL);

  simgrid_run();

  return 0;
}
