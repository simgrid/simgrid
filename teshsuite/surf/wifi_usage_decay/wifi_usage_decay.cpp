/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/config.hpp"
#include "xbt/log.h"

#include "src/surf/network_wifi.hpp"

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
    XBT_INFO("However, decay model specify that for 2 stations, we have 54Mbps become 49.00487");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/1 * 1.05/49.00487Mbps ] = 46671305");
    XBT_INFO("  simulation_time = 1000*8 / mu = 0.0001714115 (rounded to 0.000171s in SimGrid)");
  } else {
    XBT_INFO("1/r_STA1 * rho_STA1 <= 1  (there is no cross-traffic)");
    XBT_INFO("However, decay model specify that for 2 stations, we have 54Mbps become 49.00487");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/1 * 1/49.00487Mbps ] = 49004870");
    XBT_INFO("  simulation_time = 1000*8 / mu = 0.0001632491s (rounded to 0.000163s in SimGrid)");
  }
  run_ping_test("Station 1", "node1", 1000);

  XBT_INFO("TEST: Send from a station to another station on the same AP.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("We have the following constraint for AP1:");
  if (crosstraffic) {
    XBT_INFO("1.05/r_STA1 * rho_STA1 + 1.05/r_STA2 * rho_2 <= 1     (1.05 instead of 1 because of cross-traffic)");
    XBT_INFO("However, decay model specify that for 2 stations, we have 54Mbps become 49.00487");
    XBT_INFO("We should thus have:");
    XBT_INFO("  mu = 1 / [ 1/2 * 1.05/49.00487Mbps + 1.05/49.00487Mbps ] = 46671305");
    XBT_INFO("  simulation_time = 1000*8 / [ mu / 2 ] = 0.0003428231s (rounded to 0.000343s in SimGrid)");
  } else {
    XBT_INFO("1/r_STA1 * rho_STA1 +    1/r_STA2 * rho_2 <= 1   (there is no cross-traffic)");
    XBT_INFO("However, decay model specify that for 2 stations, we have 54Mbps become 49.00487");
    XBT_INFO("  mu = 1 / [ 1/2 * 1/49.00487Mbps + 1/49.00487Mbps ] = 49004870");
    XBT_INFO("  simulation_time = 1000*8 / [ mu / 2 ] =  0.0003264982s (rounded to 0.000326s in SimGrid)");
  }
  run_ping_test("Station 1", "Station 2", 1000);
}
int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dispatcher", simgrid::s4u::Host::by_name("node1"), main_dispatcher);
  engine.run();

  return 0;
}

void run_ping_test(const char* src, const char* dest, int data_size)
{
  auto* mailbox = simgrid::s4u::Mailbox::by_name("Test");

  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name(src), [mailbox, dest, data_size]() {
    double start_time = simgrid::s4u::Engine::get_clock();
    static char message[] = "message";
    mailbox->put(message, data_size);
    double end_time = simgrid::s4u::Engine::get_clock();
    XBT_INFO("Actual result: Sending %d bytes from '%s' to '%s' takes %f seconds.", data_size,
             simgrid::s4u::this_actor::get_host()->get_cname(), dest, end_time - start_time);
  });
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name(dest), [mailbox]() { mailbox->get(); });
  auto* l = (simgrid::kernel::resource::NetworkWifiLink*)simgrid::s4u::Link::by_name("AP1")->get_impl();
  if(!l->toggle_decay_model())
    l->toggle_decay_model();
  l->set_host_rate(simgrid::s4u::Host::by_name("Station 1"), 0);
  l->set_host_rate(simgrid::s4u::Host::by_name("Station 2"), 0);
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("\n");
}
