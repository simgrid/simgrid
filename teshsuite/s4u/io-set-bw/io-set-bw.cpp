/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(io_set_bw, "Messages specific for this simulation");

static void io(const sg4::Disk* disk)
{
  double cur_time = sg4::Engine::get_clock();
  disk->read(1e6);
  XBT_INFO("Read finished. Took: %lf", sg4::Engine::get_clock() - cur_time);
  cur_time = sg4::Engine::get_clock();
  disk->write(1e6);
  XBT_INFO("Write finished. Took: %lf", sg4::Engine::get_clock() - cur_time);
}

static void host()
{
  auto* disk = sg4::Host::current()->get_disks()[0];
  XBT_INFO("I/O operations: size 1e6. Should take 1s each");
  io(disk);

  XBT_INFO("Setting read limit to half (.5e6). Read should take 2s, write still 1s");
  disk->set_read_bandwidth(.5e6);
  io(disk);

  XBT_INFO("Setting write limit to half (.5e6). Write should take 2s, read still 1s");
  disk->set_read_bandwidth(1e6);
  disk->set_write_bandwidth(.5e6);
  io(disk);

  XBT_INFO("Setting readwrite limit to half (.5e6). Write and read should take 2s now");
  disk->set_readwrite_bandwidth(.5e6);
  disk->set_write_bandwidth(1e6);
  io(disk);

  disk->set_readwrite_bandwidth(1e6);
  sg4::IoPtr act;
  double cur_time;
  XBT_INFO("Change bandwidth in the middle of I/O operation");
  XBT_INFO("Setting read limit to half (.5e6) in the middle of IO. Read should take 1.5s");
  cur_time = sg4::Engine::get_clock();
  act      = disk->read_async(1e6);
  sg4::this_actor::sleep_for(.5);
  disk->set_read_bandwidth(.5e6);
  act->wait();
  XBT_INFO("Read finished. Took: %lf", sg4::Engine::get_clock() - cur_time);
  disk->set_read_bandwidth(1e6);

  XBT_INFO("Setting write limit to half (.5e6) in the middle of IO. Write should take 1.5s");
  cur_time = sg4::Engine::get_clock();
  act      = disk->write_async(1e6);
  sg4::this_actor::sleep_for(.5);
  disk->set_write_bandwidth(.5e6);
  act->wait();
  XBT_INFO("Write finished. Took: %lf", sg4::Engine::get_clock() - cur_time);
  disk->set_write_bandwidth(1e6);

  XBT_INFO("Setting readwrite limit to half (.5e6) in the middle of IO. Read and write should take 1.5s");
  cur_time        = sg4::Engine::get_clock();
  act             = disk->write_async(.5e6);
  sg4::IoPtr act2 = disk->read_async(.5e6);
  sg4::this_actor::sleep_for(.5);
  disk->set_readwrite_bandwidth(.5e6);
  act->wait();
  act2->wait();
  XBT_INFO("Read and write finished. Took: %lf", sg4::Engine::get_clock() - cur_time);
}

/*************************************************************************************************/
int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  /* simple platform containing 1 host and 2 disk */
  auto* zone = sg4::create_full_zone("bob_zone");
  auto* bob  = zone->create_host("bob", 1e6);
  auto* disk = bob->create_disk("bob_disk", 1e3, 1e3);
  /* manually setting before seal */
  disk->set_read_bandwidth(1e6);
  disk->set_write_bandwidth(1e6);
  disk->set_readwrite_bandwidth(1e6);
  zone->seal();

  sg4::Actor::create("", bob, host);

  e.run();

  return 0;
}
