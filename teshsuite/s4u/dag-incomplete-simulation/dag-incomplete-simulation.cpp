/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(incomplete, "Incomplete DAG test");

/* Dag ncomplete simulation Test
 * Scenario:
 *   - Create a bunch of activities
 *   - schedule only a subset of them (init, A and D)
 *   - run the simulation
 *   - Verify that we detect which activities are blocked and show their state.
 * The scheduled activity A sends 1GB. Simulation time should be
 *          1e9/1.25e8 + 1e-4 = 8.0001 seconds
 * Activity D is scheduled but depends on unscheduled activity C.
 */
int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  auto host = e.host_by_name("cpu0");
  /* creation of the tasks and their dependencies */
  simgrid::s4u::ExecPtr Init = simgrid::s4u::Exec::init()->set_name("Init")->set_flops_amount(0)->vetoable_start();
  simgrid::s4u::CommPtr A = simgrid::s4u::Comm::sendto_init()->set_name("A")->set_payload_size(1e9)->vetoable_start();
  simgrid::s4u::CommPtr B = simgrid::s4u::Comm::sendto_init()->set_name("B")->vetoable_start();
  simgrid::s4u::ExecPtr C = simgrid::s4u::Exec::init()->set_name("C")->vetoable_start();
  simgrid::s4u::CommPtr D = simgrid::s4u::Comm::sendto_init()->set_name("D")->set_payload_size(1e9)->vetoable_start();
  std::vector<simgrid::s4u::ActivityPtr> activities = {Init, A, B, C, D};

  Init->add_successor(A);
  Init->add_successor(B);
  C->add_successor(D);

  Init->set_host(host);
  A->set_from(host)->set_to(host);
  D->set_from(host)->set_to(host);

  /* let's launch the simulation! */
  e.run();

  int count = 0;
  for (auto a : activities) {
    if (a->get_state() == simgrid::s4u::Activity::State::STARTING) {
      count++;
      XBT_INFO("Activity '%s' blocked: Dependencies: %s; Ressources: %s", a->get_cname(),
               (a->dependencies_solved() ? "solved" : "NOT solved"), (a->is_assigned() ? "assigned" : "NOT assigned"));
    }
  }
  XBT_INFO("Simulation is finished but %d tasks are still not done", count);
  // Clean up
  C->unref();
  return 0;
}
