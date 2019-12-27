/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrate the actor migrations.
 *
 * The worker actor first move by itself, and then start an execution.
 * During that execution, the monitor migrates the worker, that wakes up on another host.
 * The execution was of the right amount of flops to take exactly 5 seconds on the first host
 * and 5 other seconds on the second one, so it stops after 10 seconds.
 *
 * Then another migration is done by the monitor while the worker is suspended.
 *
 * Note that worker() takes an uncommon set of parameters,
 * and that this is perfectly accepted by create().
 */

#include <simgrid/s4u.hpp>
#include <simgrid/s4u/Mutex.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_migration, "Messages specific for this s4u example");

static void worker(simgrid::s4u::Host* first, const simgrid::s4u::Host* second)
{
  double flopAmount = first->get_speed() * 5 + second->get_speed() * 5;

  XBT_INFO("Let's move to %s to execute %.2f Mflops (5sec on %s and 5sec on %s)", first->get_cname(), flopAmount / 1e6,
           first->get_cname(), second->get_cname());

  simgrid::s4u::this_actor::set_host(first);
  simgrid::s4u::this_actor::execute(flopAmount);

  XBT_INFO("I wake up on %s. Let's suspend a bit", simgrid::s4u::this_actor::get_host()->get_cname());

  simgrid::s4u::this_actor::suspend();

  XBT_INFO("I wake up on %s", simgrid::s4u::this_actor::get_host()->get_cname());
  XBT_INFO("Done");
}

static void monitor()
{
  simgrid::s4u::Host* boivin    = simgrid::s4u::Host::by_name("Boivin");
  simgrid::s4u::Host* jacquelin = simgrid::s4u::Host::by_name("Jacquelin");
  simgrid::s4u::Host* fafard    = simgrid::s4u::Host::by_name("Fafard");

  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("worker", fafard, worker, boivin, jacquelin);

  simgrid::s4u::this_actor::sleep_for(5);

  XBT_INFO("After 5 seconds, move the process to %s", jacquelin->get_cname());
  actor->set_host(jacquelin);

  simgrid::s4u::this_actor::sleep_until(15);
  XBT_INFO("At t=15, move the process to %s and resume it.", fafard->get_cname());
  actor->set_host(fafard);
  actor->resume();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("monitor", simgrid::s4u::Host::by_name("Boivin"), monitor);
  e.run();

  return 0;
}
