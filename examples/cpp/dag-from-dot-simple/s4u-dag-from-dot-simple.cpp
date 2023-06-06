/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_from_dot_simple, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_dot(argv[2]);

  simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
  simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");
  simgrid::s4u::Host* fafard   = e.host_by_name("Fafard");

  dynamic_cast<simgrid::s4u::Exec*>(dag[0].get())->set_host(fafard);
  dynamic_cast<simgrid::s4u::Exec*>(dag[1].get())->set_host(tremblay);
  dynamic_cast<simgrid::s4u::Exec*>(dag[2].get())->set_host(jupiter);
  dynamic_cast<simgrid::s4u::Exec*>(dag[3].get())->set_host(jupiter);
  dynamic_cast<simgrid::s4u::Exec*>(dag[8].get())->set_host(jupiter);

  for (const auto& a : dag) {
    if (auto* comm = dynamic_cast<simgrid::s4u::Comm*>(a.get())) {
      const auto* pred = dynamic_cast<simgrid::s4u::Exec*>((*comm->get_dependencies().begin()).get());
      const auto* succ = dynamic_cast<simgrid::s4u::Exec*>(comm->get_successors().front().get());
      comm->set_source(pred->get_host())->set_destination(succ->get_host());
    }
  }

  simgrid::s4u::Exec::on_completion_cb([](simgrid::s4u::Exec const& exec) {
    XBT_INFO("Exec '%s' is complete (start time: %f, finish time: %f)", exec.get_cname(),
             exec.get_start_time(), exec.get_finish_time());
  });

  simgrid::s4u::Comm::on_completion_cb([](simgrid::s4u::Comm const& comm) {
    XBT_INFO("Comm '%s' is complete (start time: %f, finish time: %f)", comm.get_cname(),
             comm.get_start_time(), comm.get_finish_time());
  });

  e.run();
  return 0;
}
