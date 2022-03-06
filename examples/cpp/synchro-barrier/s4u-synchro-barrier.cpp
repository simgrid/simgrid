/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");
namespace sg4 = simgrid::s4u;

/// Wait on the barrier then leave
static void worker(sg4::BarrierPtr barrier)
{
    XBT_INFO("Waiting on the barrier");
    barrier->wait();

    XBT_INFO("Bye");
}

/// Spawn actor_count-1 workers and do a barrier with them
static void master(int actor_count)
{
  sg4::BarrierPtr barrier = sg4::Barrier::create(actor_count);

  XBT_INFO("Spawning %d workers", actor_count - 1);
  for (int i = 0; i < actor_count - 1; i++) {
    sg4::Actor::create("worker", sg4::Host::by_name("Jupiter"), worker, barrier);
  }

    XBT_INFO("Waiting on the barrier");
    barrier->wait();

    XBT_INFO("Bye");
}

int main(int argc, char **argv)
{
  sg4::Engine e(&argc, argv);

  // Parameter: Number of actores in the barrier
  xbt_assert(argc >= 2, "Usage: %s <actor-count>\n", argv[0]);
  int actor_count = std::stoi(argv[1]);
  xbt_assert(actor_count > 0, "<actor-count> must be greater than 0");

  e.load_platform(argc > 2 ? argv[2] : "../../platforms/two_hosts.xml");
  sg4::Actor::create("master", e.host_by_name("Tremblay"), master, actor_count);
  e.run();

  return 0;
}
