/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_dependent, "Messages specific for this s4u example");

static void sender(sg4::Mailbox* mailbox)
{
  auto* computation_amount = new double(sg4::this_actor::get_host()->get_speed());
  sg4::ExecPtr exec        = sg4::this_actor::exec_init(2 * (*computation_amount));
  sg4::CommPtr comm        = mailbox->put_init(computation_amount, 7e6);

  exec->set_name("exec on sender")->add_successor(comm)->start();
  comm->set_name("comm to receiver")->start();
  exec->wait();
  comm->wait();
}

static void receiver(sg4::Mailbox* mailbox)
{
  double* received           = nullptr;
  double computation_amount  = sg4::this_actor::get_host()->get_speed();
  sg4::ExecPtr exec          = sg4::this_actor::exec_init(2 * computation_amount);
  sg4::CommPtr comm          = mailbox->get_init()->set_dst_data((void**)&received, sizeof(double));

  comm->set_name("comm from sender")->add_successor(exec)->start();
  exec->set_name("exec on receiver")->start();

  comm->wait();
  exec->wait();
  XBT_INFO("Received: %.0f flops were computed on sender", *received);
  delete received;
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Mailbox* mbox = e.mailbox_by_name_or_create("Mailbox");

  e.host_by_name("Tremblay")->add_actor("sender", sender, mbox);
  e.host_by_name("Jupiter")->add_actor("receiver", receiver, mbox);

  e.run();

  XBT_INFO("Simulation time: %.3f", e.get_clock());

  return 0;
}
