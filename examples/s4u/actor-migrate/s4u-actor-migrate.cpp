/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

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
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_migration, "Messages specific for this s4u example");

static void worker(sg4::Host* first, const sg4::Host* second)
{
  double flopAmount = first->get_speed() * 5 + second->get_speed() * 5;

  XBT_INFO("Let's move to %s to execute %.2f Mflops (5sec on %s and 5sec on %s)", first->get_cname(), flopAmount / 1e6,
           first->get_cname(), second->get_cname());

  sg4::this_actor::set_host(first);
  sg4::this_actor::execute(flopAmount);

  XBT_INFO("I wake up on %s. Let's suspend a bit", sg4::this_actor::get_host()->get_cname());

  sg4::this_actor::suspend();

  XBT_INFO("I wake up on %s", sg4::this_actor::get_host()->get_cname());
  XBT_INFO("Done");
}

static void monitor()
{
  sg4::Host* boivin    = sg4::Host::by_name("Boivin");
  sg4::Host* jacquelin = sg4::Host::by_name("Jacquelin");
  sg4::Host* fafard    = sg4::Host::by_name("Fafard");

  sg4::ActorPtr actor = sg4::Actor::create("worker", fafard, worker, boivin, jacquelin);

  sg4::this_actor::sleep_for(5);

  XBT_INFO("After 5 seconds, move the actor to %s", jacquelin->get_cname());
  actor->set_host(jacquelin);

  sg4::this_actor::sleep_until(15);
  XBT_INFO("At t=15, move the actor to %s and resume it.", fafard->get_cname());
  actor->set_host(fafard);
  actor->resume();
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);
  e.load_platform(argv[1]);

  sg4::Actor::create("monitor", sg4::Host::by_name("Boivin"), monitor);
  e.run();

  return 0;
}
