/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <float.h>
XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this example");

static void sender_fun()
{
  XBT_INFO("Sending");
  std::string* payload = new std::string("Blah");
  simgrid::s4u::Mailbox::by_name("Tremblay")->put(payload, 0);
  simgrid::s4u::this_actor::sleep_for(1.0);
  XBT_INFO("Exiting");
}

static void receiver_fun()
{
  XBT_INFO("Receiving");
  void* payload              = nullptr;
  simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("Tremblay")->get_async(&payload);
  comm->wait();
  xbt_assert(comm->get_sender(), "No sender received");
  XBT_INFO("Got a message sent by '%s'", comm->get_sender()->get_cname());
  simgrid::s4u::this_actor::sleep_for(2.0);
  XBT_INFO("Did I tell you that I got a message sent by '%s'?", comm->get_sender()->get_cname());
  delete static_cast<std::string*>(payload);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("send", simgrid::s4u::Host::by_name("Tremblay"), sender_fun);
  simgrid::s4u::Actor::create("receive", simgrid::s4u::Host::by_name("Tremblay"), receiver_fun);

  e.run();
  return 0;
}
