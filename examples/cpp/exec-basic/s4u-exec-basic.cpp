/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void executor()
{
  /* this_actor::execute() tells SimGrid to pause the calling actor
   * until its host has computed the amount of flops passed as a parameter */
  sg4::this_actor::execute(98095);
  XBT_INFO("Done.");

  /* This simple example does not do anything beyond that */
}

static void privileged()
{
  /* This version of this_actor::execute() specifies that this execution gets a larger share of the resource.
   *
   * Since the priority is 2, it computes twice as fast as a regular one.
   *
   * So instead of a half/half sharing between the two executions, we get a 1/3 vs 2/3 sharing. */
  sg4::this_actor::execute(98095, 2);
  XBT_INFO("Done.");

  /* Note that the timings printed when running this example are a bit misleading, because the uneven sharing only  last
   * until the privileged actor ends. After this point, the unprivileged one gets 100% of the CPU and finishes quite
   * quickly. */
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("executor", executor);
  e.host_by_name("Tremblay")->add_actor("privileged", privileged);

  e.run();

  return 0;
}
