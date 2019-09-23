/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/log.h"

#include "simgrid/msg.h"
#include "src/surf/network_cm02.hpp"
#include <exception>
#include <iostream>
#include <random>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[usage] wifi_usage <platform-file>");

void run_ping_test(const char* src, const char* dest, int data_size);

/* We need a separate actor so that it can sleep after each test */
static void main_dispatcher()
{
  /* Send from a station to a node on the wired network after the AP
   * The theory says:
   *   - AP1 is the limiting constraint
   *   - When two STA communicates on the same AP we have the following AP constraint:
   *     w/o cross-traffic:    1/r_STA1 * rho_STA1 <= 1
   *     with cross-traffic:   1.05/r_STA1 * rho_STA1 <= 1
   *   - Thus without cross-traffic:
   *      mu = 1 / [ 1/1 * 1/54Mbps ] = 5.4e+07
   *      simulation_time = 1000*8 / mu = 0.0001481481s
   *   - Thus with cross-traffic:
   *      mu = 1 / [ 1/1 * 1.05/54Mbps ] = 51428571
   *      simulation_time = 1000*8 / mu = 0.0001555556s (rounded to 0.000156s in SimGrid)
   */
  run_ping_test("Station 1", "NODE1", 1000);

  /* Send from a station to another station of the same AP
   * The theory says:
   *   - When two STA communicates on the same AP we have the following AP constraint:
   *     w/o cross-traffic:       1/r_STA1 * rho_STA1 +    1/r_STA2 * rho_2 <= 1
   *     with cross-traffic:   1.05/r_STA1 * rho_STA1 + 1.05/r_STA2 * rho_2 <= 1
   *   - Thus without cross-traffic:
   *      mu = 1 / [ 1/2 * 1/54Mbps + 1/54Mbps ] = 5.4e+07
   *      simulation_time = 1000*8 / [ mu / 2 ] = 0.0002962963s
   *   - Thus with cross-traffic:
   *      mu = 1 / [ 1/2 * 1.05/54Mbps + 1.05/54Mbps ] =  51428571
   *      simulation_time = 1000*8 / [ mu / 2 ] = 0.0003111111s
   */
  run_ping_test("Station 1", "Station 2", 1000);
}
int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dispatcher", simgrid::s4u::Host::by_name("NODE1"), main_dispatcher);
  engine.run();

  return 0;
}

void run_ping_test(const char* src, const char* dest, int data_size)
{
  static int test_count = 0;
  simgrid::s4u::this_actor::sleep_until(10 * test_count);
  test_count++;

  auto* mailbox = simgrid::s4u::Mailbox::by_name("Test");

  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name(src), [mailbox, dest, data_size]() {
    double start_time = simgrid::s4u::Engine::get_clock();
    mailbox->put(const_cast<char*>("message"), data_size);
    double end_time = simgrid::s4u::Engine::get_clock();
    XBT_INFO("Sending %d bytes from '%s' to '%s' takes %f seconds.", data_size,
             simgrid::s4u::this_actor::get_host()->get_cname(), dest, end_time - start_time);
  });
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name(dest), [mailbox]() { mailbox->get(); });
  auto* l = (simgrid::kernel::resource::NetworkWifiLink*)simgrid::s4u::Link::by_name("AP1")->get_impl();
  l->set_host_rate(simgrid::s4u::Host::by_name(src), 0);
}
