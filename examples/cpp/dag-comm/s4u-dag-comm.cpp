/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

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
  e.load_platform(argv[1]);

  auto* tremblay = e.host_by_name("Tremblay");
  auto* jupiter  = e.host_by_name("Jupiter");

  // Display the details on vetoed activities
  sg4::Exec::on_veto_cb([](sg4::Exec const& exec) {
    XBT_INFO("Execution '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_cname(),
             (exec.dependencies_solved() ? "solved" : "NOT solved"), (exec.is_assigned() ? "assigned" : "NOT assigned"));
  });
  sg4::Comm::on_veto_cb([](sg4::Comm const& comm) {
    XBT_INFO("Communication '%s' vetoed. Dependencies: %s; Ressources: %s", comm.get_cname(),
             (comm.dependencies_solved() ? "solved" : "NOT solved"), (comm.is_assigned() ? "assigned" : "NOT assigned"));
  });

  sg4::Exec::on_completion_cb([](sg4::Exec const& exec) {
    XBT_INFO("Exec '%s' is complete (start time: %f, finish time: %f)", exec.get_cname(), exec.get_start_time(),
             exec.get_finish_time());
  });
  sg4::Comm::on_completion_cb([](sg4::Comm const& comm) {
    XBT_INFO("Comm '%s' is complete", comm.get_cname());
  });

  // Create a small DAG: parent->transfer->child
  sg4::ExecPtr parent   = sg4::Exec::init();
  sg4::CommPtr transfer = sg4::Comm::sendto_init();
  sg4::ExecPtr child    = sg4::Exec::init();
  parent->add_successor(transfer);
  transfer->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + parent and child end after 1 second
  parent->set_name("parent")->set_flops_amount(tremblay->get_speed())->start();
  transfer->set_name("transfer")->set_payload_size(125e6)->start();
  child->set_name("child")->set_flops_amount(jupiter->get_speed())->start();

  // Schedule the different activities
  parent->set_host(tremblay);
  transfer->set_source(tremblay);
  child->set_host(jupiter);
  transfer->set_destination(jupiter);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
