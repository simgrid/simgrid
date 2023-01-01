/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/config.hpp"
#include "xbt/log.h"

#include "src/kernel/resource/WifiLinkImpl.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[usage] wifi_usage <platform-file>");

void run_ping_test(const std::vector<std::pair<std::string,std::string>>& mboxes, int data_size);

/* We need a separate actor so that it can sleep after each test */
static void main_dispatcher()
{
  const std::vector<std::pair<std::string, std::string>> flows = {
    {"Station 1", "Station 2"},
    {"Station 3", "Station 4"},
    {"Station 5", "Station 6"},
    {"Station 7", "Station 8"},
    {"Station 9", "Station 10"},
    {"Station 11", "Station 12"},
    {"Station 13", "Station 14"},
    {"Station 15", "Station 16"},
    {"Station 17", "Station 18"},
    {"Station 19", "Station 20"},
    {"Station 21", "Station 22"},
  };

  XBT_INFO("1/r_STA1 * rho_STA1 <= 1  (there is no cross-traffic)");
  XBT_INFO("22 concurrent flows, decay model deactivated, we have 54Mbps to share between the flows");
  XBT_INFO("We should thus have:");
  XBT_INFO("  mu = 1 / [ 1/22 * (1/54Mbps)*22 ] = 54000000");
  XBT_INFO("  simulation_time = 100000*8 / (mu/22) = 0.3259259259259259 (rounded to 0.325926s in SimGrid)");

  run_ping_test(flows, 100000);

  XBT_INFO("1/r_STA1 * rho_STA1 <= 1  (there is no cross-traffic)");
  XBT_INFO("22 concurrent flows, decay model activated, we have 54Mbps to share between the flows, but the number of concurrent flows is above the limit (20)");
  XBT_INFO("We should thus have:");
  XBT_INFO("Maximum throughput of the link reduced by:");
  XBT_INFO("updated link capacity = ( 5678270 + (22-20) * -5424 ) / 5678270 =~ 0.998086");
  XBT_INFO("  mu = 1 / [ 1/22 * (1/54Mbps*0.998086)*22 ] = 53896644");
  XBT_INFO("  simulation_time = 100000*8 / (mu/22) = 0.3265509444335718 (rounded to 0.326550 in SimGrid)");

  auto* l = (simgrid::kernel::resource::WifiLinkImpl*)simgrid::s4u::Link::by_name("AP1")->get_impl();
  l->toggle_callback();
  run_ping_test(flows, 100000);
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dispatcher", engine.host_by_name("node1"), main_dispatcher);
  engine.run();

  return 0;
}

void run_ping_test(const std::vector<std::pair<std::string,std::string>>& mboxes, int data_size)
{
  auto* mailbox = simgrid::s4u::Mailbox::by_name("Test");
  for (auto const& pair : mboxes) {
    simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name(pair.first.c_str()), [mailbox, pair, data_size]() {
      double start_time          = simgrid::s4u::Engine::get_clock();
      static std::string message = "message";
      mailbox->put(&message, data_size);
      double end_time = simgrid::s4u::Engine::get_clock();
      XBT_INFO("Actual result: Sending %d bytes from '%s' to '%s' takes %f seconds.", data_size,
              simgrid::s4u::this_actor::get_host()->get_cname(), pair.second.c_str(), end_time - start_time);
    });
    simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name(pair.second.c_str()),
                                [mailbox]() { mailbox->get<std::string>(); });
    auto* l = (simgrid::kernel::resource::WifiLinkImpl*)simgrid::s4u::Link::by_name("AP1")->get_impl();
    for(auto i=1; i<=22; i++) {
      l->set_host_rate(simgrid::s4u::Host::by_name("Station "+std::to_string(i)), 0);
    }
  }
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("\n");
}
