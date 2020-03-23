/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* This example implements one process which receives messages from two other */
/* processes. There is no bug on it, it is just provided to test the soundness*/
/* of the state space reduction with DPOR, if the maximum depth (defined with */
/* --cfg=model-check/max-depth:) is reached.                                  */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(electric_fence, "Example to check the soundness of DPOR");

static void server()
{
  void* data1                          = nullptr;
  void* data2                          = nullptr;
  simgrid::s4u::CommPtr comm_received1 = simgrid::s4u::Mailbox::by_name("mymailbox")->get_async(&data1);
  simgrid::s4u::CommPtr comm_received2 = simgrid::s4u::Mailbox::by_name("mymailbox")->get_async(&data2);

  comm_received1->wait();
  comm_received2->wait();

  XBT_INFO("OK");
  delete static_cast<int*>(data1);
  delete static_cast<int*>(data2);
}

static void client(int id)
{
  int* payload = new int();
  *payload     = id;
  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload, 10000);
  XBT_INFO("Sent!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("server", simgrid::s4u::Host::by_name("HostA"), server);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostB"), client, 1);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostC"), client, 2);

  e.run();
  return 0;
}
