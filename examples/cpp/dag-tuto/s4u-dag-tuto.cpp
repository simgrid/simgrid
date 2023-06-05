/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_tuto, "Messages specific for this s4u tutorial");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
  simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");

  simgrid::s4u::ExecPtr c1 = simgrid::s4u::Exec::init();
  simgrid::s4u::ExecPtr c2 = simgrid::s4u::Exec::init();
  simgrid::s4u::ExecPtr c3 = simgrid::s4u::Exec::init();
  simgrid::s4u::CommPtr t1 = simgrid::s4u::Comm::sendto_init();

  c1->set_name("c1");
  c2->set_name("c2");
  c3->set_name("c3");
  t1->set_name("t1");

  c1->set_flops_amount(1e9);
  c2->set_flops_amount(5e9);
  c3->set_flops_amount(2e9);
  t1->set_payload_size(5e8);

  c1->add_successor(t1);
  t1->add_successor(c3);
  c2->add_successor(c3);

  c1->set_host(tremblay);
  c2->set_host(jupiter);
  c3->set_host(jupiter);
  t1->set_source(tremblay);
  t1->set_destination(jupiter);

  c1->start();
  c2->start();

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
