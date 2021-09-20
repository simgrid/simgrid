/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this example");

static void master()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("comm");
  simgrid::s4u::Host* jupiter    = simgrid::s4u::Host::by_name("Jupiter");

  XBT_INFO("Master starting");
  simgrid::s4u::this_actor::sleep_for(0.5);

  auto* payload              = new std::string("COMM");
  simgrid::s4u::CommPtr comm = mailbox->put_async(payload, 1E8);

  simgrid::s4u::this_actor::sleep_for(0.5);

  XBT_INFO("Turning off the worker host");
  jupiter->turn_off();

  try {
    comm->wait();
  } catch (const simgrid::NetworkFailureException&) {
    delete payload;
  }

  XBT_INFO("Master has finished");
}

static void worker()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("comm");

  XBT_INFO("Worker receiving");
  try {
    auto payload = mailbox->get_unique<std::string>();
    XBT_DEBUG("Received message: %s", payload->c_str());
  } catch (const simgrid::HostFailureException&) {
    XBT_DEBUG("The host has been turned off, this was expected");
    return;
  }

  XBT_ERROR("Worker should be off already.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("master", e.host_by_name("Tremblay"), master);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Jupiter"), worker);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
