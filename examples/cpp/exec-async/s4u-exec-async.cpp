/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

/* This actor simply waits for its activity completion after starting it.
 * That's exactly equivalent to synchronous execution. */
static void waiter()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();
  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg4::ExecPtr activity = sg4::this_actor::exec_init(computation_amount);
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

/* This actor tests the ongoing execution until its completion, and don't wait before it's terminated. */
static void monitor()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();
  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg4::ExecPtr activity = sg4::this_actor::exec_init(computation_amount);
  activity->start();

  while (not activity->test()) {
    XBT_INFO("Remaining amount of flops: %g (%.0f%%)", activity->get_remaining(),
             100 * activity->get_remaining_ratio());
    sg4::this_actor::sleep_for(0.3);
  }

  XBT_INFO("Goodbye now!");
}

/* This actor cancels the ongoing execution after a while. */
static void canceller()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();

  XBT_INFO("Execute %g flops, should take 1 second.", computation_amount);
  sg4::ExecPtr activity = sg4::this_actor::exec_async(computation_amount);
  sg4::this_actor::sleep_for(0.5);
  XBT_INFO("I changed my mind, cancel!");
  activity->cancel();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Host* fafard  = e.host_by_name("Fafard");
  sg4::Host* ginette = e.host_by_name("Ginette");
  sg4::Host* boivin  = e.host_by_name("Boivin");

  sg4::Actor::create("wait", fafard, waiter);
  sg4::Actor::create("monitor", ginette, monitor);
  sg4::Actor::create("cancel", boivin, canceller);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
