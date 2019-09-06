/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

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

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[wifi_usage] 1STA-1LINK-1NODE");

void setup_simulation();
static void flowActor(std::vector<std::string> args);

/**
 * Theory says:
 *   - AP1 is the most constraint constraint
 *   - When two STA communicates on the same AP we have the following AP constraint:
 *     1/r_STA1 * rho_STA1 <= 1
 *   - Thus:
 *      mu = 1 / [ 1/1 * 1/54Mbps ] = 5.4e+07
 *      simulation_time = 1000*8 / mu = 0.0001481481s
 *
 */
int main(int argc, char** argv)
{

  // Build engine
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  setup_simulation();
  engine.run();
  XBT_INFO("Simulation took %fs", simgrid::s4u::Engine::get_clock());

  return (0);
}

void setup_simulation()
{

  std::vector<std::string> args, noArgs;
  args.push_back("NODE1");
  args.push_back("1000");
  simgrid::s4u::Actor::create("STA1", simgrid::s4u::Host::by_name("STA1"), flowActor, args);
  simgrid::s4u::Actor::create("NODE1", simgrid::s4u::Host::by_name("NODE1"), flowActor, noArgs);
  simgrid::kernel::resource::NetworkWifiLink* l =
      (simgrid::kernel::resource::NetworkWifiLink*)simgrid::s4u::Link::by_name("AP1")->get_impl();
  l->set_host_rate(simgrid::s4u::Host::by_name("STA1"), 0);
}

static void flowActor(std::vector<std::string> args)
{
  std::string selfName               = simgrid::s4u::this_actor::get_host()->get_name();
  simgrid::s4u::Mailbox* selfMailbox = simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_host()->get_name());

  if (args.size() > 0) { // We should send
    simgrid::s4u::Mailbox* dstMailbox = simgrid::s4u::Mailbox::by_name(args.at(0));

    int dataSize        = std::atoi(args.at(1).c_str());
    double comStartTime = simgrid::s4u::Engine::get_clock();
    dstMailbox->put(const_cast<char*>("message"), dataSize);
    double comEndTime = simgrid::s4u::Engine::get_clock();
    XBT_INFO("%s sent %d bytes to %s in %f seconds from %f to %f", selfName.c_str(), dataSize, args.at(0).c_str(),
             comEndTime - comStartTime, comStartTime, comEndTime);
  } else { // We should receive
    selfMailbox->get();
  }
}
