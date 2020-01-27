/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <simgrid/plugins/file_system.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test()
{
  simgrid::s4u::ExecPtr bob_compute = simgrid::s4u::this_actor::exec_init(1e9);
  simgrid::s4u::IoPtr bob_write =
      simgrid::s4u::Host::current()->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::WRITE);
  simgrid::s4u::IoPtr carl_read =
      simgrid::s4u::Host::by_name("carl")->get_disks().front()->io_init(4000000, simgrid::s4u::Io::OpType::READ);
  simgrid::s4u::ExecPtr carl_compute = simgrid::s4u::Host::by_name("carl")->exec_async(1e9);

  // Name the activities (for logging purposes only)
  bob_compute->set_name("bob compute");
  bob_write->set_name("bob write");
  carl_read->set_name("carl read");
  carl_compute->set_name("carl compute");

  // Create the dependencies:
  // 'bob_write' is a successor of 'bob_compute'
  // 'carl_read' is a successor of 'bob_write'
  // 'carl_compute' is a successor of 'carl_read'
  bob_compute->add_successor(bob_write.get());
  bob_write->add_successor(carl_read.get());
  carl_read->add_successor(carl_compute.get());

  // Start the activities.
  bob_compute->start();
  bob_write->vetoable_start();
  carl_read->vetoable_start();
  carl_compute->vetoable_start();

  // Wait for their completion (should be replaced by a wait_any_for at some point)
  bob_compute->wait();
  bob_write->wait();
  carl_read->wait();
  carl_compute->wait();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("bob", simgrid::s4u::Host::by_name("bob"), test);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
