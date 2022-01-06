/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void writer()
{
  /* - Retrieve all disks from current host */
  std::vector<simgrid::s4u::Disk*> const& disk_list = simgrid::s4u::Host::current()->get_disks();
  /* - Write 4,000,000 bytes on Disk1 */
  disk_list.front()->write(4000000);
  XBT_INFO("First write done.");
  /* - Write 4,000,000 bytes on Disk1 again */
  disk_list.front()->write(4000000);
  XBT_INFO("Second write done.");
}

static void privileged_writer()
{
  /* - Retrieve all disks from current host */
  std::vector<simgrid::s4u::Disk*> const& disk_list = simgrid::s4u::Host::current()->get_disks();

  /* - Write 4,000,000 bytes on Disk1 but specifies that this I/O operation gets a larger share of the resource.
   *
   * Since the priority is 2, it writes twice as fast as a regular one.
   *
   * So instead of a half/half sharing between the two, we get a 1/3 vs. 2/3 sharing. */
  disk_list.front()->write(4000000, 2);
  XBT_INFO("First write done.");

  /* Note that the timings printed when running this example are a bit misleading, because the uneven sharing only last
   * until the privileged actor ends. After this point, the unprivileged one gets 100% of the disk and finishes quite
   * quickly. */

  /* Resynchronize actors before second write */
  simgrid::s4u::this_actor::sleep_for(0.05);

  /* - Write 4,000,000 bytes on Disk1 again and this time :
   *    - Start the I/O operation asynchronously to get an IoPtr
   *    - Let the actor sleep while half of the data is written
   *    - Double its priority
   *    - Wait for the end of the I/O operation
   *
   *   Writing the second half of the data with a higher priority, and thus 2/3 of the bandwidth takes 0.075s.
   *   In the meantime, the regular writer has only 1/3 of the bandwidth and thus only writes 1MB. After the completion
   *   of the I/O initiated by the privileged writer, the regular writer has the full bandwidth available and only needs
   *   0.025s to write the last MB.
   */

  simgrid::s4u::IoPtr io = disk_list.front()->write_async(4000000);
  simgrid::s4u::this_actor::sleep_for(0.1);
  XBT_INFO("Increase priority for the priviledged writer (%.0f bytes remaining to write)", io->get_remaining());
  io->update_priority(2);
  io->wait();
  XBT_INFO("Second write done.");
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
