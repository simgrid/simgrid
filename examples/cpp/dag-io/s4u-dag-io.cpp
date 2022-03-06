/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  auto bob  = e.host_by_name("bob");
  auto carl = e.host_by_name("carl");

  // Display the details on vetoed activities
  sg4::Activity::on_veto_cb([](const sg4::Activity& a) {
    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", a.get_cname(),
             (a.dependencies_solved() ? "solved" : "NOT solved"), (a.is_assigned() ? "assigned" : "NOT assigned"));
  });

  sg4::Activity::on_completion_cb([](sg4::Activity const& activity) {
    const auto* exec = dynamic_cast<sg4::Exec const*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec->get_cname(), exec->get_start_time(),
             exec->get_finish_time());
  });

  // Create a small DAG: parent->write_output->read_input->child
  sg4::ExecPtr parent     = sg4::Exec::init();
  sg4::IoPtr write_output = sg4::Io::init();
  sg4::IoPtr read_input   = sg4::Io::init();
  sg4::ExecPtr child      = sg4::Exec::init();
  parent->add_successor(write_output);
  write_output->add_successor(read_input);
  read_input->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + parent and chile end after 1 second
  parent->set_name("parent")->set_flops_amount(bob->get_speed());
  write_output->set_name("write")->set_size(1e9)->set_op_type(sg4::Io::OpType::WRITE);
  read_input->set_name("read")->set_size(1e9)->set_op_type(sg4::Io::OpType::READ);
  child->set_name("child")->set_flops_amount(carl->get_speed());

  // Schedule and try to start the different activities
  parent->set_host(bob)->vetoable_start();
  write_output->set_disk(bob->get_disks().front())->vetoable_start();
  read_input->set_disk(carl->get_disks().front())->vetoable_start();
  child->set_host(carl)->vetoable_start();

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
