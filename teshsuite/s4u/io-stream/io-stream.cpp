/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(io_stream, "Messages specific for this simulation");

static void streamer(size_t size)
{
  auto* bob        = sg4::Host::by_name("bob");
  auto* alice      = sg4::Host::by_name("alice");
  const auto* bob_disk   = bob->get_disks().front();
  const auto* alice_disk = alice->get_disks().front();
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
  size_t block_size = size / 100;
  sg4::IoPtr read       = bob_disk->read_async(block_size);
  sg4::CommPtr transfer = sg4::Comm::sendto_async(bob, alice, block_size);
  sg4::IoPtr write      = alice_disk->write_async(block_size);

  clock = sg4::Engine::get_clock();

  for (int i = 0; i < 99; i++){
    read->wait();
    read = bob_disk->read_async(block_size);
    transfer->wait();
    transfer = sg4::Comm::sendto_async(bob, alice, block_size);
    write->wait();
    write = alice_disk->write_async(block_size);
  }

  read->wait();
  transfer->wait();
  write->wait();
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("[Bob -> Alice] Streaming (Read bottleneck)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, bob_disk, alice, alice_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("[Alice -> Bob] Streaming (Write bottleneck)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(alice, alice_disk, bob, bob_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("Start two 10-second background traffic between Bob and Alice");
  sg4::CommPtr bt1 = sg4::Comm::sendto_async(bob, alice, 2e7);
  XBT_INFO("[Bob -> Alice] Streaming (Transfer bottleneck)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, bob_disk, alice, alice_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);
  bt1->wait();

  XBT_INFO("[Bob -> Alice] Streaming \"from disk to memory\" (no write)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, bob_disk, alice, nullptr, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("[Bob -> Alice] Streaming \"from memory to disk\" (no read)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, nullptr, alice, alice_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);

  XBT_INFO("[Bob -> Bob] Disk to disk (no transfer)");
  clock = sg4::Engine::get_clock();
  sg4::Io::streamto(bob, bob_disk, bob, bob_disk, size);
  XBT_INFO("    Total : %.6f seconds", sg4::Engine::get_clock() - clock);
}

static void background_send() {
  sg4::this_actor::sleep_for(23.000150);
  sg4::Mailbox::by_name("mbox")->put(new double(1), 2e7);
}

static void background_recv() {
  double* res;
  sg4::CommPtr comm = sg4::Mailbox::by_name("mbox")->get_async<double>(&res);
  sg4::this_actor::sleep_for(33.1);
  comm->wait();
  delete res;
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  /* simple platform containing 2 hosts and 2 disks */
  auto* zone = sg4::create_full_zone("");
  auto* bob  = zone->create_host("bob", 1e6);
  auto* alice  = zone->create_host("alice", 1e6);

  auto* link = zone->create_link("link", "2MBps")->set_latency("50us");
  zone->add_route(bob, alice, {link});

  bob->create_disk("bob_disk", "1MBps", "500kBps");
  alice->create_disk("alice_disk", "4MBps", "4MBps");

  zone->seal();

  sg4::Actor::create("streamer", bob, streamer, 4e6);
  sg4::Actor::create("background send", bob, background_send);
  sg4::Actor::create("background recv", alice, background_recv);

  e.run();

  return 0;
}
