/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/**
 * Test for issue105: https://framagit.org/simgrid/simgrid/-/issues/105
 */

#include <simgrid/kernel/ProfileBuilder.hpp>
#include <simgrid/s4u.hpp>
#include <xbt/log.h>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(issue105, "Issue105");
static void load_generator(sg4::Mailbox* mailbox)
{
  std::vector<sg4::CommPtr> comms;

  // Send the task messages
  for (int i = 0; i < 100; i++) {
    auto* payload     = new int(i);
    sg4::CommPtr comm = mailbox->put_async(payload, 1024);
    comms.push_back(comm);
    sg4::this_actor::sleep_for(1.0);
  }

  // send shutdown messages
  auto* payload     = new int(-1);
  sg4::CommPtr comm = mailbox->put_async(payload, 1024);
  XBT_INFO("Sent shutdown");
  comms.push_back(comm);

  // Wait for all messages to be consumed before ending the simulation
  sg4::Comm::wait_all(comms);
  XBT_INFO("Load generator finished");
}

static void receiver(sg4::Mailbox* mailbox)
{
  bool shutdown = false;
  while (not shutdown) {
    auto msg = mailbox->get_unique<int>();
    if (*msg >= 0) {
      XBT_INFO("Started Task: %d", *msg);
      sg4::this_actor::execute(1e9);
    } else {
      shutdown = true;
    }
  }
  XBT_INFO("Receiver finished");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  sg4::NetZone* world = sg4::create_full_zone("world");
  sg4::Host* hostGl01 = world->create_host("host-gl01", "98Mf")->seal();
  sg4::Host* hostSa01 = world->create_host("host-sa01", "98Mf")->seal();

  // create latency and bandwidth profiles
  auto* linkSaLatencyProfile   = simgrid::kernel::profile::ProfileBuilder::from_string("link-sa-latency-profile",
                                                                                     "0 0.100\n"
                                                                                     "0.102 0.00002\n",
                                                                                     60);
  auto* linkSaBandwidthProfile = simgrid::kernel::profile::ProfileBuilder::from_string("link-sa-bandwidth-profile",
                                                                                       "0 42000000\n"
                                                                                       "1 200\n"
                                                                                       "100 42000000\n",
                                                                                       150);
  const sg4::Link* linkSa      = world->create_link("link-front-sa", "42MBps")
                                ->set_latency("20ms")
                                ->set_latency_profile(linkSaLatencyProfile)
                                ->set_bandwidth_profile(linkSaBandwidthProfile)
                                ->seal();

  world->add_route(hostGl01->get_netpoint(), hostSa01->get_netpoint(), nullptr, nullptr,
                   {{linkSa, sg4::LinkInRoute::Direction::NONE}}, true);
  world->seal();

  sg4::Mailbox* mb1 = e.mailbox_by_name_or_create("Mailbox 1");
  sg4::Actor::create("load-generator", hostGl01, load_generator, mb1);
  sg4::Actor::create("cluster-node-sa01", hostSa01, receiver, mb1);

  e.run();

  XBT_INFO("Total simulation time: %.3f", e.get_clock());
  return 0;
}