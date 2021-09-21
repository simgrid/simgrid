/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* This example implements one actor which receives messages from two other   */
/* actors. There is no bug on it, it is just provided to test the soundness   */
/* of the state space reduction with DPOR, if the maximum depth (defined with */
/* --cfg=model-check/max-depth:) is reached.                                  */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(electric_fence, "Example to check the soundness of DPOR");

namespace sg4 = simgrid::s4u;

static void server()
{
  int* data1                  = nullptr;
  int* data2                  = nullptr;
  sg4::CommPtr comm_received1 = sg4::Mailbox::by_name("mymailbox")->get_async<int>(&data1);
  sg4::CommPtr comm_received2 = sg4::Mailbox::by_name("mymailbox")->get_async<int>(&data2);

  comm_received1->wait();
  comm_received2->wait();

  XBT_INFO("OK");
  delete data1;
  delete data2;
}

static void client(int id)
{
  auto* payload = new int(id);
  sg4::Mailbox::by_name("mymailbox")->put(payload, 10000);
  XBT_INFO("Sent!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("server", e.host_by_name("HostA"), server);
  sg4::Actor::create("client", e.host_by_name("HostB"), client, 1);
  sg4::Actor::create("client", e.host_by_name("HostC"), client, 2);

  e.run();
  return 0;
}
