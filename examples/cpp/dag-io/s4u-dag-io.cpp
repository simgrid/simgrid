/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  auto bob  = e.host_by_name("bob");
  auto carl = e.host_by_name("carl");

  // Display the details on vetoed activities
  simgrid::s4u::Activity::on_veto.connect([](simgrid::s4u::Activity& a) {
    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", a.get_cname(),
             (a.dependencies_solved() ? "solved" : "NOT solved"), (a.is_assigned() ? "assigned" : "NOT assigned"));
  });

  simgrid::s4u::Exec::on_completion.connect([](simgrid::s4u::Exec const& exec) {
    XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec.get_cname(), exec.get_start_time(),
             exec.get_finish_time());
  });

  // Create a small DAG: parent->write_output->read_input->child
  simgrid::s4u::ExecPtr parent     = simgrid::s4u::Exec::init();
  simgrid::s4u::IoPtr write_output = simgrid::s4u::Io::init();
  simgrid::s4u::IoPtr read_input   = simgrid::s4u::Io::init();
  simgrid::s4u::ExecPtr child      = simgrid::s4u::Exec::init();
  parent->add_successor(write_output);
  write_output->add_successor(read_input);
  read_input->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + parent and chile end after 1 second
  parent->set_name("parent")->set_flops_amount(bob->get_speed());
  write_output->set_name("write")->set_size(1e9)->set_op_type(simgrid::s4u::Io::OpType::WRITE);
  read_input->set_name("read")->set_size(1e9)->set_op_type(simgrid::s4u::Io::OpType::READ);
  child->set_name("child")->set_flops_amount(carl->get_speed());

  // Schedule and try to start the different activities
  parent->set_host(bob)->vetoable_start();
  write_output->set_disk(bob->get_disks().front())->vetoable_start();
  read_input->set_disk(carl->get_disks().front())->vetoable_start();
  child->set_host(carl)->vetoable_start();

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
