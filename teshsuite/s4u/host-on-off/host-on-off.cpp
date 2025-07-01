/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("jupi");

  while (true) {
    std::unique_ptr<std::string> payload;
    try {
      payload = mailbox->get_unique<std::string>();
    } catch (const simgrid::HostFailureException&) {
      XBT_DEBUG("The host has been turned off, this was expected");
      return;
    }

    if (*payload == "finalize") {
      break;
    }
    simgrid::s4u::this_actor::execute(5E7);

    XBT_INFO("Task \"%s\" done", payload->c_str());
  }
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("jupi");
  simgrid::s4u::Host* jupiter    = simgrid::s4u::Host::by_name("Jupiter");

  auto* payload = new std::string("task on");

  XBT_INFO("Sending \"task on\"");
  mailbox->put_async(payload, 1E6)->wait_for_or_cancel(1);

  simgrid::s4u::this_actor::sleep_for(1);
  jupiter->turn_off();

  XBT_INFO("Sending \"task off\"");
  payload = new std::string("task off");
  try {
    mailbox->put_async(payload, 1E6)->wait_for_or_cancel(1);
  } catch (const simgrid::TimeoutException&) {
    delete payload;
  }

  jupiter->turn_on();

  std::vector<simgrid::s4u::ActorPtr> jupi_actors = jupiter->get_all_actors();
  for (const auto& actor : jupi_actors)
    actor->kill();

  XBT_INFO("Sending \"task on without actor\"");
  payload = new std::string("task on without actor");
  try {
    mailbox->put_async(payload, 1E6)->wait_for_or_cancel(1);
  } catch (const simgrid::TimeoutException&) {
    delete payload;
  }

  simgrid::s4u::Host::by_name("Jupiter")->add_actor("worker", worker);

  XBT_INFO("Sending \"task on with actor\"");
  payload = new std::string("task on with actor");
  try {
    mailbox->put_async(payload, 1E6)->wait_for_or_cancel(1);
  } catch (const simgrid::TimeoutException&) {
    delete payload;
  }

  XBT_INFO("Sending \"finalize\"");
  payload = new std::string("finalize");
  try {
    mailbox->put_async(payload, 0)->wait_for_or_cancel(1);
  } catch (const simgrid::TimeoutException&) {
    delete payload;
  }

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("master", master);
  e.host_by_name("Jupiter")->add_actor("worker", worker);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
