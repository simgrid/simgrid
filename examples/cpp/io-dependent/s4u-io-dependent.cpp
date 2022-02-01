/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <simgrid/plugins/file_system.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test()
{
  std::vector<simgrid::s4u::ActivityPtr> pending_activities;

  simgrid::s4u::ExecPtr bob_compute = simgrid::s4u::this_actor::exec_init(1e9);
  pending_activities.push_back(boost::dynamic_pointer_cast<simgrid::s4u::Activity>(bob_compute));
  simgrid::s4u::IoPtr bob_write =
      simgrid::s4u::Host::current()->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::WRITE);
  pending_activities.push_back(boost::dynamic_pointer_cast<simgrid::s4u::Activity>(bob_write));
  simgrid::s4u::IoPtr carl_read =
      simgrid::s4u::Host::by_name("carl")->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::READ);
  pending_activities.push_back(boost::dynamic_pointer_cast<simgrid::s4u::Activity>(carl_read));
  simgrid::s4u::ExecPtr carl_compute = simgrid::s4u::Host::by_name("carl")->exec_init(1e9);
  pending_activities.push_back(boost::dynamic_pointer_cast<simgrid::s4u::Activity>(carl_compute));

  // Name the activities (for logging purposes only)
  bob_compute->set_name("bob compute");
  bob_write->set_name("bob write");
  carl_read->set_name("carl read");
  carl_compute->set_name("carl compute");

  // Create the dependencies:
  // 'bob_write' is a successor of 'bob_compute'
  // 'carl_read' is a successor of 'bob_write'
  // 'carl_compute' is a successor of 'carl_read'
  bob_compute->add_successor(bob_write);
  bob_write->add_successor(carl_read);
  carl_read->add_successor(carl_compute);

  // Start the activities.
  bob_compute->start();
  bob_write->vetoable_start();
  carl_read->vetoable_start();
  carl_compute->vetoable_start();

  // wait for the completion of all activities
  while (not pending_activities.empty()) {
    ssize_t changed_pos = simgrid::s4u::Activity::wait_any(pending_activities);
    XBT_INFO("Activity '%s' is complete", pending_activities[changed_pos]->get_cname());
    pending_activities.erase(pending_activities.begin() + changed_pos);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("bob", e.host_by_name("bob"), test);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
