/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_dependent, "Messages specific for this s4u example");

static void sender(simgrid::s4u::Mailbox* mailbox)
{
  auto* computation_amount   = new double(simgrid::s4u::this_actor::get_host()->get_speed());
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(2 * (*computation_amount));
  simgrid::s4u::CommPtr comm = mailbox->put_init(computation_amount, 7e6);

  exec->set_name("exec on sender")->add_successor(comm)->start();
  comm->set_name("comm to receiver")->vetoable_start();
  exec->wait();
  comm->wait();
}

static void receiver(simgrid::s4u::Mailbox* mailbox)
{
  double* received           = nullptr;
  double computation_amount  = simgrid::s4u::this_actor::get_host()->get_speed();
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(2 * computation_amount);
  simgrid::s4u::CommPtr comm = mailbox->get_init()->set_dst_data((void**)&received, sizeof(double));

  comm->set_name("comm from sender")->add_successor(exec)->start();
  exec->set_name("exec on receiver")->vetoable_start();

  comm->wait();
  exec->wait();
  XBT_INFO("Received: %.0f flops were computed on sender", *received);
  delete received;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("Mailbox");

  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("Tremblay"), sender, mbox);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("Jupiter"), receiver, mbox);

  e.run();

  XBT_INFO("Simulation time: %.3f", e.get_clock());

  return 0;
}
