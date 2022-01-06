/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/**
 * Test NetworkCm02Link::set_bandwidth and set_latency.
 *
 * Math behind: https://hal.inria.fr/inria-00361031/document
 * - Fig. 1 and Eq. 7 in the paper
 *
 *
 * Platform: dogbone
 *
 *   S1 ___[ 1 ]___                 ___[ 2 ]___ C1
 *                 \               /
 *                  R1 __[ 0 ]__ R2
 *                 /               \
 *   S2 ___[ 3 ]__/                 \__[ 4 ]___ C2
 *
 * 2 communications:
 * - Flow A: S1 -> C1
 * - Flow B: S2 -> C2
 *
 * Links: [1], [2], [3]: 1Gb/s, 10ms
 * Link: [0] bottleneck: 1Mb/s, 20ms
 * Link: [4]: We'll change the bandwidth and latency to see the impact on the
 * sharing of the bottleneck link between the 2 flows.
 */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(cm02_set_lat_bw, "Messages specific for this simulation");

static void sender(const std::string& recv_name, sg4::Link* l4)
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name(recv_name);
  XBT_INFO("Comm to %s, same weight/penalty (w_a == w_b, ~20) for everybody, each comm should take 1s and finish at "
           "the same time",
           recv_name.c_str());
  auto* payload = new double(sg4::Engine::get_clock());
  auto comm     = mbox->put_async(payload, 1e3);
  comm->wait();
  sg4::this_actor::sleep_until(10); // synchronize senders

  if (recv_name == "C2") {
    XBT_INFO("Comm Flow B to C2: after 1s, change latency of L4 to increase penalty for flow B (w_b = 2* w_a)");
    XBT_INFO("rho_a = 2*rho_b, flow A receives twice the bandwidth than flow B");
  } else {
    XBT_INFO("Comm Flow A to C1");
  }
  payload = new double(sg4::Engine::get_clock());
  comm    = mbox->put_async(payload, 1e3);
  sg4::this_actor::sleep_for(1);
  if (l4)
    l4->set_latency(20);
  comm->wait();
  sg4::this_actor::sleep_until(20); // synchronize senders
  if (l4)
    l4->set_latency(1e-9);

  if (recv_name == "C2") {
    XBT_INFO("Comm Flow B to C2: after 1s, change bandwidth of L4 to increase penalty for flow B (w_b = 2* w_a)");
    XBT_INFO("rho_a = 2*rho_b, flow A receives twice the bandwidth than flow B");
  } else {
    XBT_INFO("Comm Flow A to C1");
  }
  payload = new double(sg4::Engine::get_clock());
  comm    = mbox->put_async(payload, 1e3);
  sg4::this_actor::sleep_for(1);
  if (l4)
    l4->set_bandwidth(1e3);
  comm->wait();
  sg4::this_actor::sleep_until(30);

  payload = new double(-1.0);
  mbox->put(payload, 0);
}

static void receiver()
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
  while (true) {
    auto payload = mbox->get_unique<double>();
    if (*payload < 0)
      break;
    XBT_INFO("Received data. Elapsed %lf", sg4::Engine::get_clock() - *payload);
  }
  XBT_INFO("Bye");
}

/*************************************************************************************************/
int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  /* keep it simple, no network factors nor crosstrafic */
  sg4::Engine::set_config("network/model:CM02");
  sg4::Engine::set_config("network/weight-S:20537");
  sg4::Engine::set_config("network/crosstraffic:0");

  /* dog-bone platform */
  std::unordered_map<std::string, sg4::Host*> hosts;
  std::unordered_map<std::string, sg4::Link*> links;
  auto* zone = sg4::create_full_zone("dog_zone");
  for (const auto& name : {"S1", "S2", "C1", "C2"}) {
    hosts[name] = zone->create_host(name, 1e6)->seal();
  }

  for (const auto& name : {"L1", "L2", "L3", "L4"}) {
    links[name] = zone->create_link(name, 1e9)->set_latency(1e-9)->seal();
  }
  links["L0"] = zone->create_link("L0", 1e3)->seal();
  zone->add_route(hosts["S1"]->get_netpoint(), hosts["C1"]->get_netpoint(), nullptr, nullptr,
                  {sg4::LinkInRoute(links["L1"]), sg4::LinkInRoute(links["L0"]), sg4::LinkInRoute(links["L2"])});
  zone->add_route(hosts["S2"]->get_netpoint(), hosts["C2"]->get_netpoint(), nullptr, nullptr,
                  {sg4::LinkInRoute(links["L3"]), sg4::LinkInRoute(links["L0"]), sg4::LinkInRoute(links["L4"])});

  zone->seal();

  sg4::Actor::create("", hosts["S1"], sender, "C1", nullptr);
  sg4::Actor::create("", hosts["C1"], receiver);
  sg4::Actor::create("", hosts["S2"], sender, "C2", links["L4"]);
  sg4::Actor::create("", hosts["C2"], receiver);

  e.run();

  return 0;
}
