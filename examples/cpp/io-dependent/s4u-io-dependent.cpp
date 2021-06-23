/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <simgrid/plugins/file_system.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test()
{
  std::vector<simgrid::s4u::IoPtr> pending_ios;

  simgrid::s4u::ExecPtr bob_compute = simgrid::s4u::this_actor::exec_init(1e9);
  simgrid::s4u::IoPtr bob_write =
      simgrid::s4u::Host::current()->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::WRITE);
  pending_ios.push_back(bob_write);
  simgrid::s4u::IoPtr carl_read =
      simgrid::s4u::Host::by_name("carl")->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::READ);
  pending_ios.push_back(carl_read);
  simgrid::s4u::ExecPtr carl_compute = simgrid::s4u::Host::by_name("carl")->exec_init(1e9);

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
  bob_compute->wait();
  while (not pending_ios.empty()) {
    ssize_t changed_pos = simgrid::s4u::Io::wait_any(pending_ios);
    XBT_INFO("Io '%s' is complete", pending_ios[changed_pos]->get_cname());
    pending_ios.erase(pending_ios.begin() + changed_pos);
  }
  carl_compute->wait();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("bob", simgrid::s4u::Host::by_name("bob"), test);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
