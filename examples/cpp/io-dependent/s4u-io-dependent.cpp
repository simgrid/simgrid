/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <simgrid/plugins/file_system.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void test()
{
  sg4::ExecPtr bob_compute = sg4::this_actor::exec_init(1e9);
  sg4::IoPtr bob_write = sg4::Host::current()->get_disks().front()->io_init(4000000, sg4::Io::OpType::WRITE);
  sg4::IoPtr carl_read = sg4::Host::by_name("carl")->get_disks().front()->io_init(4000000, sg4::Io::OpType::READ);
  sg4::ExecPtr carl_compute = sg4::Host::by_name("carl")->exec_init(1e9);

  sg4::ActivitySet pending_activities ({bob_compute, bob_write, carl_read, carl_compute});

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
  bob_write->start();
  carl_read->start();
  carl_compute->start();

  // wait for the completion of all activities
  while (not pending_activities.empty()) {
    auto completed_one = pending_activities.wait_any();
    if (completed_one != nullptr)
      XBT_INFO("Activity '%s' is complete", completed_one->get_cname());
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Actor::create("bob", e.host_by_name("bob"), test);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
