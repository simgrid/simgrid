/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/config.hpp"
#include "xbt/log.h"

#include "src/kernel/resource/WifiLinkImpl.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[usage] wifi_usage <platform-file>");

void run_ping_test(const char* src, const char* dest, int data_size);

/* We need a separate actor so that it can sleep after each test */
static void main_dispatcher()
{
  bool crosstraffic = simgrid::kernel::resource::NetworkModel::cfg_crosstraffic;

  XBT_INFO("TEST: Send from a station to a node on the wired network after the AP.");
  XBT_INFO("----------------------------------------------------------------------");
  XBT_INFO("Since AP1 is the limiting link, we have the following constraint for AP1:");
  if (crosstraffic) {
    XBT_INFO("1.05/r_STA1 * rho_STA1 <= 1   (1.05 instead of 1 because of cross-traffic)");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/1 * 1.05/54Mbps ] = 51428571");
    XBT_INFO("  simulation_time = 1000*8 / mu = 0.0001555556s (rounded to 0.000156s in SimGrid)");
  } else {
    XBT_INFO("1/r_STA1 * rho_STA1 <= 1  (there is no cross-traffic)");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/1 * 1/54Mbps ] = 5.4e+07");
    XBT_INFO("  simulation_time = 1000*8 / mu = 0.0001481481s");
  }
  run_ping_test("Station 1", "node1", 1000);

  XBT_INFO("TEST: Send from a station to another station on the same AP.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("We have the following constraint for AP1:");
  if (crosstraffic) {
    XBT_INFO("1.05/r_STA1 * rho_STA1 + 1.05/r_STA2 * rho_2 <= 1     (1.05 instead of 1 because of cross-traffic)");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/2 * 1.05/54Mbps + 1.05/54Mbps ] =  51428571");
    XBT_INFO("  simulation_time = 1000*8 / [ mu / 2 ] = 0.0003111111s");
  } else {
    XBT_INFO("1/r_STA1 * rho_STA1 +    1/r_STA2 * rho_2 <= 1   (there is no cross-traffic)");
    XBT_INFO("  mu = 1 / [ 1/2 * 1/54Mbps + 1/54Mbps ] = 5.4e+07");
    XBT_INFO("  simulation_time = 1000*8 / [ mu / 2 ] = 0.0002962963s");
  }
  run_ping_test("Station 1", "Station 2", 1000);
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dispatcher", engine.host_by_name("node1"), main_dispatcher);
  engine.run();

  return 0;
}

void run_ping_test(const char* src, const char* dest, int data_size)
{
  auto* mailbox = simgrid::s4u::Mailbox::by_name("Test");

  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name(src), [mailbox, dest, data_size]() {
    double start_time          = simgrid::s4u::Engine::get_clock();
    static std::string message = "message";
    mailbox->put(&message, data_size);
    double end_time = simgrid::s4u::Engine::get_clock();
    XBT_INFO("Actual result: Sending %d bytes from '%s' to '%s' takes %f seconds.", data_size,
             simgrid::s4u::this_actor::get_host()->get_cname(), dest, end_time - start_time);
  });
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name(dest),
                              [mailbox]() { mailbox->get<std::string>(); });
  const auto* ap1 = simgrid::s4u::Link::by_name("AP1");
  ap1->set_host_wifi_rate(simgrid::s4u::Host::by_name(src), 0);
  ap1->set_host_wifi_rate(simgrid::s4u::Host::by_name(dest), 0);
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("\n");
}
