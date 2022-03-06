/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void test(sg_size_t size)
{
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  XBT_INFO("Hello! read %llu bytes from %s", size, disk->get_cname());

  sg4::IoPtr activity = disk->io_init(size, sg4::Io::OpType::READ);
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

static void test_waitfor(sg_size_t size)
{
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  XBT_INFO("Hello! write %llu bytes from %s", size, disk->get_cname());

  sg4::IoPtr activity = disk->write_async(size);
  try {
    activity->wait_for(0.5);
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Asynchronous write: Timeout!");
  }

  XBT_INFO("Goodbye now!");
}

static void test_cancel(sg_size_t size)
{
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  sg4::this_actor::sleep_for(0.5);
  XBT_INFO("Hello! write %llu bytes from %s", size, disk->get_cname());

  sg4::IoPtr activity = disk->write_async(size);
  sg4::this_actor::sleep_for(0.5);
  XBT_INFO("I changed my mind, cancel!");
  activity->cancel();

  XBT_INFO("Goodbye now!");
}

static void test_monitor(sg_size_t size)
{
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  sg4::this_actor::sleep_for(1);
  sg4::IoPtr activity = disk->write_async(size);

  while (not activity->test()) {
    XBT_INFO("Remaining amount of bytes to write: %g", activity->get_remaining());
    sg4::this_actor::sleep_for(0.2);
  }
  activity->wait();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg4::Actor::create("test", e.host_by_name("bob"), test, 2e7);
  sg4::Actor::create("test_waitfor", e.host_by_name("alice"), test_waitfor, 5e7);
  sg4::Actor::create("test_cancel", e.host_by_name("alice"), test_cancel, 5e7);
  sg4::Actor::create("test_monitor", e.host_by_name("alice"), test_monitor, 5e7);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
