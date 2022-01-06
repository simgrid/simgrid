/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

/// Wait on the barrier then leave
static void worker(simgrid::s4u::BarrierPtr barrier)
{
    XBT_INFO("Waiting on the barrier");
    barrier->wait();

    XBT_INFO("Bye");
}

/// Spawn actor_count-1 workers and do a barrier with them
static void master(int actor_count)
{
  simgrid::s4u::BarrierPtr barrier = simgrid::s4u::Barrier::create(actor_count);

  XBT_INFO("Spawning %d workers", actor_count - 1);
  for (int i = 0; i < actor_count - 1; i++) {
    simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), worker, barrier);
  }

    XBT_INFO("Waiting on the barrier");
    barrier->wait();

    XBT_INFO("Bye");
}

int main(int argc, char **argv)
{
  simgrid::s4u::Engine e(&argc, argv);

  // Parameter: Number of actores in the barrier
  xbt_assert(argc >= 2, "Usage: %s <actor-count>\n", argv[0]);
  int actor_count = std::stoi(argv[1]);
  xbt_assert(actor_count > 0, "<actor-count> must be greater than 0");

  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("master", e.host_by_name("Tremblay"), master, actor_count);
  e.run();

  return 0;
}
