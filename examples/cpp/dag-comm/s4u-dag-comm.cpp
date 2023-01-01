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
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  auto tremblay = e.host_by_name("Tremblay");
  auto jupiter  = e.host_by_name("Jupiter");

  // Display the details on vetoed activities
  sg4::Activity::on_veto_cb([](const sg4::Activity& a) {
    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", a.get_cname(),
             (a.dependencies_solved() ? "solved" : "NOT solved"), (a.is_assigned() ? "assigned" : "NOT assigned"));
  });

  sg4::Activity::on_completion_cb([](sg4::Activity const& activity) {
    if (const auto* exec = dynamic_cast<sg4::Exec const*>(&activity))
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec->get_cname(), exec->get_start_time(),
               exec->get_finish_time());
    if (const auto* comm = dynamic_cast<sg4::Comm const*>(&activity))
      XBT_INFO("Activity '%s' is complete", comm->get_cname());
  });

  // Create a small DAG: parent->transfer->child
  sg4::ExecPtr parent   = sg4::Exec::init();
  sg4::CommPtr transfer = sg4::Comm::sendto_init();
  sg4::ExecPtr child    = sg4::Exec::init();
  parent->add_successor(transfer);
  transfer->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + parent and child end after 1 second
  parent->set_name("parent")->set_flops_amount(tremblay->get_speed())->vetoable_start();
  transfer->set_name("transfer")->set_payload_size(125e6)->vetoable_start();
  child->set_name("child")->set_flops_amount(jupiter->get_speed())->vetoable_start();

  // Schedule the different activities
  parent->set_host(tremblay);
  transfer->set_source(tremblay);
  child->set_host(jupiter);
  transfer->set_destination(jupiter);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
