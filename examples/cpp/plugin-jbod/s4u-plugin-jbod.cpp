/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/jbod.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(jbod_test, "Messages specific for this simulation");
namespace sg4 = simgrid::s4u;

static void write_then_read(simgrid::plugin::JbodPtr jbod)
{
  simgrid::plugin::JbodIoPtr io = jbod->write_async(1e7);
  XBT_INFO("asynchronous write posted, wait for it");
  io->wait();
  XBT_INFO("asynchronous write done");
  jbod->read(1e7);
  XBT_INFO("synchonous read done");
  jbod->write(1e7);
  XBT_INFO("synchonous write done");
  io = jbod->read_async(1e7);
  XBT_INFO("asynchronous read posted, wait for it");
  io->wait();
  XBT_INFO("asynchonous read done");
  jbod->write(1e7);
  XBT_INFO("synchonous write done");
  jbod->read(1e7);
  XBT_INFO("synchonous read done");
  jbod->read(1e7);
  XBT_INFO("synchonous read done");
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  auto* zone = sg4::create_full_zone("zone");
  auto* host = zone->create_host("host", "1Gf");
  // set up link so that data transfer from host to JBOD takes exactly 1 second (without crosstraffic)
  auto* link = zone->create_link("link", 1e7/0.97)->set_latency(0);

  auto jbod_raid0 =
      simgrid::plugin::Jbod::create_jbod(zone, "jbod_raid0", 1e9, 4, simgrid::plugin::Jbod::RAID::RAID0, 1e7, 5e6);
  zone->add_route(host, jbod_raid0->get_controller(), {link});

  auto jbod_raid1 =
      simgrid::plugin::Jbod::create_jbod(zone, "jbod_raid1", 1e9, 4, simgrid::plugin::Jbod::RAID::RAID1, 1e7, 5e6);
  zone->add_route(host, jbod_raid1->get_controller(), {link});

  auto jbod_raid4 =
      simgrid::plugin::Jbod::create_jbod(zone, "jbod_raid4", 1e9, 4, simgrid::plugin::Jbod::RAID::RAID4, 1e7, 5e6);
  zone->add_route(host, jbod_raid4->get_controller(), {link});

  auto jbod_raid5 =
      simgrid::plugin::Jbod::create_jbod(zone, "jbod_raid5", 1e9, 4, simgrid::plugin::Jbod::RAID::RAID5, 1e7, 5e6);
  zone->add_route(host, jbod_raid5->get_controller(), {link});

  auto jbod_raid6 =
      simgrid::plugin::Jbod::create_jbod(zone, "jbod_raid6", 1e9, 4, simgrid::plugin::Jbod::RAID::RAID6, 1e7, 5e6);
  zone->add_route(host, jbod_raid6->get_controller(), {link});

  zone->seal();

  XBT_INFO("XXXXXXXXXXXXXXX RAID 0 XXXXXXXXXXXXXXXX");
  sg4::Actor::create("", host, write_then_read, jbod_raid0);
  e.run();

  XBT_INFO("XXXXXXXXXXXXXXX RAID 1 XXXXXXXXXXXXXXXX");
  sg4::Actor::create("", host, write_then_read, jbod_raid1);
  e.run();

  XBT_INFO("XXXXXXXXXXXXXXX RAID 4 XXXXXXXXXXXXXXXX");
  sg4::Actor::create("", host, write_then_read, jbod_raid4);
  e.run();

  XBT_INFO("XXXXXXXXXXXXXXX RAID 5 XXXXXXXXXXXXXXXX");
  sg4::Actor::create("", host, write_then_read, jbod_raid5);
  e.run();

  XBT_INFO("XXXXXXXXXXXXXXX RAID 6 XXXXXXXXXXXXXXXX");
  sg4::Actor::create("", host, write_then_read, jbod_raid6);
  e.run();

  XBT_INFO("Simulated time: %g", sg4::Engine::get_clock());

  return 0;
}
