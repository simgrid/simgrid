/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test(sg_size_t size)
{
  simgrid::s4u::Disk* disk = simgrid::s4u::Host::current()->get_disks().front();
  XBT_INFO("Hello! read %llu bytes from %s", size, disk->get_cname());

  simgrid::s4u::IoPtr activity = disk->io_init(size, simgrid::s4u::Io::OpType::READ);
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

static void test_waitfor(sg_size_t size)
{
  simgrid::s4u::Disk* disk = simgrid::s4u::Host::current()->get_disks().front();
  XBT_INFO("Hello! write %llu bytes from %s", size, disk->get_cname());

  simgrid::s4u::IoPtr activity = disk->write_async(size);
  try {
    activity->wait_for(0.5);
  } catch (simgrid::TimeoutException&) {
    XBT_INFO("Asynchronous write: Timeout!");
  }

  XBT_INFO("Goodbye now!");
}

static void test_cancel(sg_size_t size)
{
  simgrid::s4u::Disk* disk = simgrid::s4u::Host::current()->get_disks().front();
  simgrid::s4u::this_actor::sleep_for(0.5);
  XBT_INFO("Hello! write %llu bytes from %s", size, disk->get_cname());

  simgrid::s4u::IoPtr activity = disk->write_async(size);
  simgrid::s4u::this_actor::sleep_for(0.5);
  XBT_INFO("I changed my mind, cancel!");
  activity->cancel();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("test", simgrid::s4u::Host::by_name("bob"), test, 2e7);
  simgrid::s4u::Actor::create("test_waitfor", simgrid::s4u::Host::by_name("alice"), test_waitfor, 5e7);
  simgrid::s4u::Actor::create("test_cancel", simgrid::s4u::Host::by_name("alice"), test_cancel, 5e7);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
