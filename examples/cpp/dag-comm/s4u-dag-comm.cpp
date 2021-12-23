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

  auto tremblay = e.host_by_name("Tremblay");
  auto jupiter  = e.host_by_name("Jupiter");

  // Display the details on vetoed activities
  simgrid::s4u::Activity::on_veto.connect([](simgrid::s4u::Activity& a) {
    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", a.get_cname(),
             (a.dependencies_solved() ? "solved" : "NOT solved"), (a.is_assigned() ? "assigned" : "NOT assigned"));
  });

  simgrid::s4u::Activity::on_completion.connect([](simgrid::s4u::Activity& activity) {
    auto* exec = dynamic_cast<simgrid::s4u::Exec*>(&activity);
    if (exec != nullptr)
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec->get_cname(), exec->get_start_time(),
               exec->get_finish_time());
    auto* comm = dynamic_cast<simgrid::s4u::Comm*>(&activity);
    if (comm != nullptr)
      XBT_INFO("Activity '%s' is complete", comm->get_cname());
  });

  // Create a small DAG: parent->transfert->child
  simgrid::s4u::ExecPtr parent    = simgrid::s4u::Exec::init();
  simgrid::s4u::CommPtr transfert = simgrid::s4u::Comm::sendto_init();
  simgrid::s4u::ExecPtr child     = simgrid::s4u::Exec::init();
  parent->add_successor(transfert);
  transfert->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + parent and child end after 1 second
  parent->set_name("parent")->set_flops_amount(tremblay->get_speed())->vetoable_start();
  transfert->set_name("transfert")->set_payload_size(125e6)->vetoable_start();
  child->set_name("child")->set_flops_amount(jupiter->get_speed())->vetoable_start();

  // Schedule the different activities
  parent->set_host(tremblay);
  transfert->set_source(tremblay);
  child->set_host(jupiter);
  transfert->set_destination(jupiter);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
