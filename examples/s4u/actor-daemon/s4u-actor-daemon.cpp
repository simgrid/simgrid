/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_daemon, "Messages specific for this s4u example");

/* The worker actor, working for a while before leaving */
static void worker()
{
  XBT_INFO("Let's do some work (for 10 sec on Boivin).");
  simgrid::s4u::this_actor::execute(980.95e6);

  XBT_INFO("I'm done now. I leave even if it makes the daemon die.");
}

/* The daemon, displaying a message every 3 seconds until all other actors stop */
static void my_daemon()
{
  simgrid::s4u::Actor::self()->daemonize();

  while (simgrid::s4u::this_actor::get_host()->is_on()) {
    XBT_INFO("Hello from the infinite loop");
    simgrid::s4u::this_actor::sleep_for(3.0);
  }

  XBT_INFO("I will never reach that point: daemons are killed when regular actors are done");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Boivin"), worker);
  simgrid::s4u::Actor::create("daemon", simgrid::s4u::Host::by_name("Tremblay"), my_daemon);

  e.run();
  return 0;
}
