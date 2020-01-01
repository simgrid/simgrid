/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

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

/// Spawn process_count-1 workers and do a barrier with them
static void master(int process_count)
{
    simgrid::s4u::BarrierPtr barrier = simgrid::s4u::Barrier::create(process_count);

    XBT_INFO("Spawning %d workers", process_count-1);
    for (int i = 0; i < process_count-1; i++)
    {
        simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), worker, barrier);
    }

    XBT_INFO("Waiting on the barrier");
    barrier->wait();

    XBT_INFO("Bye");
}

int main(int argc, char **argv)
{
  simgrid::s4u::Engine e(&argc, argv);

  // Parameter: Number of processes in the barrier
  xbt_assert(argc >= 2, "Usage: %s <process-count>\n", argv[0]);
  int process_count = std::stoi(argv[1]);
  xbt_assert(process_count > 0, "<process-count> must be greater than 0");

  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), master, process_count);
  e.run();

  return 0;
}
