/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(io_stream, "Messages specific for this simulation");

static void streamer(size_t size)
{
  auto* bob        = sg4::Host::by_name("bob");
  auto* bob_disk   = bob->get_disks().front();
  auto* alice      = sg4::Host::by_name("alice");
  auto* alice_disk = alice->get_disks().front();
  double clock = sg4::Engine::get_clock();

  XBT_INFO("[Bob -> Alice] Store and Forward (1 block)");
  bob_disk->read(size);
  XBT_INFO("    Read  : %.6f seconds", sg4::Engine::get_clock() - clock);
  clock = sg4::Engine::get_clock();
  sg4::Comm::sendto(bob, alice, size);
  XBT_INFO("    Send  : %.6f seconds", sg4::Engine::get_clock() - clock);
  clock = sg4::Engine::get_clock();
  alice_disk->write(size);
  XBT_INFO("    Write : %.6f seconds", sg4::Engine::get_clock() - clock);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock());

  XBT_INFO("[Bob -> Alice] Store and Forward (100 blocks)");
  sg4::IoPtr read = bob_disk->read_async(size/100.0);
  sg4::CommPtr transfer = sg4::Comm::sendto_async(bob, alice, size/100.0);
  sg4::IoPtr write = alice_disk->write_async(size/100.);

  clock = sg4::Engine::get_clock();

  for (int i = 0; i < 99; i++){
    read->wait();
    read = bob_disk->read_async(size/100.0);
    transfer->wait();
    transfer = sg4::Comm::sendto_async(bob, alice, size/100.0);
    write->wait();
    write = alice_disk->write_async(size/100.);
  }

  read->wait();
  transfer->wait();
  write->wait();
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);


  XBT_INFO("[Bob -> Alice] Streaming (Read bottleneck)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, bob_disk, alice, alice_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("[Bob -> Alice] Streaming (Write bottleneck)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(alice, alice_disk, bob, bob_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  /* simple platform containing 2 hosts and 2 disks */
  auto* zone = sg4::create_full_zone("");
  auto* bob  = zone->create_host("bob", 1e6);
  auto* alice  = zone->create_host("alice", 1e6);

  sg4::LinkInRoute link(zone->create_link("link", "2MBps")->set_latency("50us")->seal());
  zone->add_route(bob->get_netpoint(), alice->get_netpoint(), nullptr, nullptr, {link}, true);

  bob->create_disk("bob_disk", 1e6, 5e5);
  alice->create_disk("alice_disk", 4e6, 4e6);

  zone->seal();

  sg4::Actor::create("streamer", bob, streamer, 4e6);

  e.run();

  return 0;
}
