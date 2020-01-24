/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  // Define an amount of work that should take 1 second to execute.
  double computation_amount = simgrid::s4u::this_actor::get_host()->get_speed();

  std::vector<simgrid::s4u::ExecPtr> pending_execs;
  // Create a small DAG
  // + Two parents and a child
  // + First parent ends after 1 second and the Second parent after 2 seconds.
  simgrid::s4u::ExecPtr first_parent  = simgrid::s4u::this_actor::exec_init(computation_amount);
  pending_execs.push_back(first_parent);
  simgrid::s4u::ExecPtr second_parent = simgrid::s4u::this_actor::exec_init(2 * computation_amount);
  pending_execs.push_back(second_parent);
  simgrid::s4u::ExecPtr child = simgrid::s4u::this_actor::exec_init(computation_amount);
  pending_execs.push_back(child);

  // Name the activities (for logging purposes only)
  first_parent->set_name("parent 1");
  second_parent->set_name("parent 2");
  child->set_name("child");

  // Create the dependencies by declaring 'child' as a successor of first_parent and second_parent
  first_parent->add_successor(child.get());
  second_parent->add_successor(child.get());

  // Start the activities.
  first_parent->start();
  second_parent->start();
  // child uses a vetoable start to force it to wait for the completion of its predecessors
  child->vetoable_start();

  // wait for the completion of all activities
  while (not pending_execs.empty()) {
    int changed_pos = simgrid::s4u::Exec::wait_any_for(&pending_execs, -1);
    XBT_INFO("Exec '%s' is complete", pending_execs[changed_pos]->get_cname());
    pending_execs.erase(pending_execs.begin() + changed_pos);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Fafard"), worker);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
