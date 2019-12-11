/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

simgrid::s4u::ExecPtr child;

static void worker()
{
  double computation_amount = simgrid::s4u::this_actor::get_host()->get_speed();

  simgrid::s4u::ExecPtr first_parent  = simgrid::s4u::this_actor::exec_init(computation_amount);
  simgrid::s4u::ExecPtr second_parent = simgrid::s4u::this_actor::exec_init(2 * computation_amount);

  child = simgrid::s4u::this_actor::exec_init(computation_amount);

  first_parent->add_successor(child.get());
  second_parent->add_successor(child.get());
  second_parent->start();
  first_parent->wait();
  second_parent->wait();
}

static void vetoed_worker()
{
  child->vetoable_start();
  while (not child->test()) {
    if (child->get_state() == simgrid::s4u::Exec::State::STARTING)
      XBT_INFO("child cannot start yet");
    else
      XBT_INFO("child is now in state %d", (int)child->get_state());
    simgrid::s4u::this_actor::sleep_for(0.25);
  }
  XBT_INFO("Should be okay now, child is in state %d", (int)child->get_state());
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
