/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void writer()
{
  /* - Retrieve all disks from current host */
  std::vector<simgrid::s4u::Disk*> const& disk_list = simgrid::s4u::Host::current()->get_disks();
  /* - Write 400,000 bytes on Disk1 */
  disk_list.front()->write(4000000);
  XBT_INFO("Done.");
}

static void privileged_writer()
{
  /* - Retrieve all disks from current host */
  std::vector<simgrid::s4u::Disk*> const& disk_list = simgrid::s4u::Host::current()->get_disks();

  /* - Write 400,000 bytes on Disk1 but specifies that this I/O operation gets a larger share of the resource.
   *
   * Since the priority is 2, it writes twice as fast as a regular one.
   *
   * So instead of a half/half sharing between the two, we get a 1/3 vs. 2/3 sharing. */
  disk_list.front()->io_init(4000000, simgrid::s4u::Io::OpType::WRITE)->set_priority(2)->wait();
  XBT_INFO("Done.");

  /* Note that the timings printed when running this example are a bit misleading, because the uneven sharing only last
   * until the privileged actor ends. After this point, the unprivileged one gets 100% of the CPU and finishes quite
   * quickly. */
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("writer", e.host_by_name("bob"), writer);
  simgrid::s4u::Actor::create("privileged_writer", e.host_by_name("bob"), privileged_writer);

  e.run();

  return 0;
}
