/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

simgrid::s4u::ExecPtr second;

static void worker()
{
  double computation_amount = simgrid::s4u::this_actor::get_host()->get_speed();

  simgrid::s4u::ExecPtr first = simgrid::s4u::this_actor::exec_init(computation_amount);
  second                      = simgrid::s4u::this_actor::exec_init(computation_amount);

  first->add_successor(second.get());
  first->start();
  first->wait();
}

static void vetoed_worker()
{
  second->vetoable_start();
  XBT_INFO("%d %d", (int)second->get_state(), second->has_dependencies());
  second->wait();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Fafard"), worker);
  simgrid::s4u::Actor::create("vetoed_worker", simgrid::s4u::Host::by_name("Fafard"), vetoed_worker);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
