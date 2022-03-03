/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(ptask_subflows_test, "Messages specific for this s4u example");

static void ptask(sg4::Host* recv, sg4::Host* sender)
{
  XBT_INFO("TEST: 1 parallel task with 2 flows");
  XBT_INFO("Parallel task sends 1.5B to other host.");
  XBT_INFO("Same result for L07 and BMF since the ptask is alone.");
  XBT_INFO("Should be done in 2.5 seconds: 1s latency and 1.5 second for transfer");

  double start_time = sg4::Engine::get_clock();
  sg4::Exec::init()
      ->set_bytes_amounts(std::vector<double>({0.0, 0.0, 1.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0}))
      ->set_hosts(std::vector<sg4::Host*>({sender, sender, recv}))
      ->wait();
  double end_time = sg4::Engine::get_clock();
  XBT_INFO("Parallel task finished after %lf seconds", end_time - start_time);

  XBT_INFO("TEST: Same parallel task but with a noisy communication at the side");
  XBT_INFO("Parallel task sends 1.5B to other host.");
  XBT_INFO("With BMF: Should be done in 3.5 seconds: 1s latency and 2 second for transfer.");
  XBT_INFO("With L07: Should be done in 4 seconds: 1s latency and 3 second for transfer.");
  XBT_INFO("With BMF, ptask gets 50%% more bandwidth than the noisy flow (because of the sub).");
  start_time = sg4::Engine::get_clock();
  auto noisy = sg4::Comm::sendto_async(sender, recv, 10);
  sg4::Exec::init()
      ->set_bytes_amounts(std::vector<double>({0.0, 0.0, 1.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0}))
      ->set_hosts(std::vector<sg4::Host*>({sender, sender, recv}))
      ->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Parallel task finished after %lf seconds", end_time - start_time);
  noisy->wait(); // gracefully wait the noisy flow
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  auto* rootzone = sg4::create_full_zone("root");
  auto* hostA    = rootzone->create_host("hostA", 1e9);
  auto* hostB    = rootzone->create_host("hostB", 1e9);
  sg4::LinkInRoute link(rootzone->create_link("backbone", "1")->set_latency("1s")->seal());
  rootzone->add_route(hostA->get_netpoint(), hostB->get_netpoint(), nullptr, nullptr, {link}, true);
  rootzone->seal();

  sg4::Actor::create("ptask", hostA, ptask, hostA, hostB);

  e.run();

  return 0;
}
