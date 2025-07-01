/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cfloat>
XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this example");

static void sender_fun()
{
  XBT_INFO("Sending");
  auto* payload = new std::string("Blah");
  simgrid::s4u::Mailbox::by_name("Tremblay")->put(payload, 0);
  simgrid::s4u::this_actor::sleep_for(1.0);
  XBT_INFO("Exiting");
}

static void receiver_fun()
{
  XBT_INFO("Receiving");
  std::string* payload       = nullptr;
  simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("Tremblay")->get_async<std::string>(&payload);
  comm->wait();
  xbt_assert(comm->get_sender(), "No sender received");
  XBT_INFO("Got a message sent by '%s'", comm->get_sender()->get_cname());
  simgrid::s4u::this_actor::sleep_for(2.0);
  XBT_INFO("Did I tell you that I got a message sent by '%s'?", comm->get_sender()->get_cname());
  delete payload;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("send", sender_fun);
  e.host_by_name("Tremblay")->add_actor("receive", receiver_fun);

  e.run();
  return 0;
}
