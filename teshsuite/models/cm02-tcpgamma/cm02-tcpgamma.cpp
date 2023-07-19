/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/**
 * Test CM02 model wrt TCP gamma
 *
 * Platform: single link
 *
 *   S1 ___[ L1 ]___S2
 *
 * Link L1: 1Gb/s, 10ms
 */

#include "src/kernel/resource/WifiLinkImpl.hpp"
#include <simgrid/s4u.hpp>
#include <xbt/config.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(cm02_tcpgamma, "Messages specific for this simulation");

static void run_ping_test(sg4::Link const* testlink)
{
  auto* mailbox = simgrid::s4u::Mailbox::by_name("Test");

  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("host1"), [mailbox, testlink]() {
    double start_time   = simgrid::s4u::Engine::get_clock();
    static auto message = std::string("message");
    mailbox->put(&message, 1e10);
    double end_time = simgrid::s4u::Engine::get_clock();
    XBT_INFO("  Actual result: Sending 10Gb with bw=%.0eb, lat=%.0es lasts %f seconds (TCP_Gamma=%.0f).",
             testlink->get_bandwidth(), testlink->get_latency(), end_time - start_time,
             simgrid::config::get_value<double>("network/TCP-gamma"));
  });
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("host2"),
                              [mailbox]() { mailbox->get<std::string>(); });
  simgrid::s4u::this_actor::sleep_for(500);
}

/* We need a separate actor so that it can sleep after each test */
static void main_dispatcher(sg4::Link* testlink)
{
  XBT_INFO("-----------------------------------------------------------");
  XBT_INFO("This test set enforces the impact of the latency and TCP-gamma parameter on the bandwidth."); 
  XBT_INFO("See the documentation about the CM02 TCP performance model.");
  XBT_INFO("-----------------------------------------------------------");
  XBT_INFO("TEST with latency = 0 sec (and the default value of Gamma):");
  XBT_INFO("  Expectation: Gamma/2lat is not defined, so the physical bandwidth is used; The communication lasts 1 sec.");
  run_ping_test(testlink);
  XBT_INFO("-----------------------------------------------------------");
  testlink->set_latency(0.00001);
  XBT_INFO("TEST with latency = 0.00001 sec");
  XBT_INFO("  Expectation: Gamma/2lat is about 209 Gb/s, which is larger than the physical bandwidth.");
  XBT_INFO("    So communication is limited by the physical bandwidth and lasts 1.00001 sec.");
  run_ping_test(testlink);
  XBT_INFO("-----------------------------------------------------------");
  testlink->set_latency(0.001);
  XBT_INFO("TEST with latency = 0.001 sec");
  XBT_INFO("  Expectation: Gamma/2lat is about 2 Gb/s, which is smaller than the physical bandwidth.");
  XBT_INFO("    So the communication is limited by the latency and lasts 4.768372 + 0.001 sec.");
  run_ping_test(testlink);
  XBT_INFO("-----------------------------------------------------------");
  testlink->set_latency(0.1);
  XBT_INFO("TEST with latency = 0.1 sec");
  XBT_INFO("  Expectation: Gamma/2lat is about 2 Gb/s, which is smaller than the physical bandwidth.");
  XBT_INFO("    So the communication is limited by the latency and lasts 476.837158 + 0.1 sec.");
  run_ping_test(testlink);
  XBT_INFO("-----------------------------------------------------------");
  XBT_INFO("TEST with latency = 0.001 sec and TCP_Gamma = 0");
  sg4::Engine::set_config("network/TCP-gamma:0");
  testlink->set_latency(0.001);
  XBT_INFO("  Expectation: The latency=0.001s should make the communication to be limited by the latency.");
  XBT_INFO("    But since gamma=0, the physical bandwidth is still used. So the communication lasts 1.001 sec.");
  run_ping_test(testlink);
  XBT_INFO("-----------------------------------------------------------");
}

int main(int argc, char** argv)
{
  sg4::Engine::set_config("network/crosstraffic:0");

  simgrid::s4u::Engine engine(&argc, argv);
  auto* zone  = sg4::create_full_zone("world");
  auto const* host1 = zone->create_host("host1", 1e6)->seal();
  auto const* host2 = zone->create_host("host2", 1e6)->seal();
  auto* testlink    = zone->create_link("L1", 1e10)->seal();
  zone->add_route(host1, host2, {testlink});

  simgrid::s4u::Actor::create("dispatcher", engine.host_by_name("host1"), main_dispatcher, testlink);
  engine.run();

  return 0;
}
