/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  simgrid::s4u::Host* other_host = simgrid::s4u::Host::by_name("Fafard");
  unsigned int first =
      simgrid::s4u::Host::on_state_change.connect([](simgrid::s4u::Host const&) { XBT_INFO("First callback"); });
  unsigned int second =
      simgrid::s4u::Host::on_state_change.connect([](simgrid::s4u::Host const&) { XBT_INFO("Second callback"); });
  unsigned int third =
      simgrid::s4u::Host::on_state_change.connect([](simgrid::s4u::Host const&) { XBT_INFO("Third callback"); });

  XBT_INFO("Turning off: Three callbacks should be triggered");
  other_host->turn_off();

  XBT_INFO("Disconnect the second callback");
  simgrid::s4u::Host::on_state_change.disconnect(second);

  XBT_INFO("Turning on: Two callbacks should be triggered");
  other_host->turn_on();

  XBT_INFO("Disconnect the first callback");
  simgrid::s4u::Host::on_state_change.disconnect(first);

  XBT_INFO("Turning off: One callback should be triggered");
  other_host->turn_off();

  XBT_INFO("Disconnect the third callback");
  simgrid::s4u::Host::on_state_change.disconnect(third);
  XBT_INFO("Turning on: No more callbacks");
  other_host->turn_on();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Tremblay"), worker);

  e.run();

  return 0;
}
