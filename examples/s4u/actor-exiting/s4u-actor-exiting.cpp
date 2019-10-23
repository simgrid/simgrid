/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* There is two very different ways of being informed when an actor exits.
 *
 * The this_actor::on_exit() function allows one to register a function to be
 * executed when this very actor exits. The registered function will run
 * when this actor terminates (either because its main function returns, or
 * because it's killed in any way). No simcall are allowed here: your actor
 * is dead already, so it cannot interact with its environment in any way
 * (network, executions, disks, etc).
 *
 * Usually, the functions registered in this_actor::on_exit() are in charge
 * of releasing any memory allocated by the actor during its execution.
 *
 * The other way of getting informed when an actor terminates is to connect a
 * function in the Actor::on_termination signal, that is shared between
 * all actors. Callbacks to this signal are executed for each terminating
 * actors, no matter what. This is useful in many cases, in particular
 * when developing SimGrid plugins.
 *
 * Finally, you can attach callbacks to the Actor::on_destruction signal.
 * It is also shared between all actors, and gets fired when the actors
 * are destroyed. A delay is possible between the termination of an actor
 * (ie, when it terminates executing its code) and its destruction (ie,
 * when it is not referenced anywhere in the simulation and can be collected).
 *
 * In both cases, you can stack more than one callback in the signal.
 * They will all be executed in the registration order.
 */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_exiting, "Messages specific for this s4u example");

static void actor_a()
{
  // Register a lambda function to be executed once it stops
  simgrid::s4u::this_actor::on_exit([](bool /*failed*/) { XBT_INFO("I stop now"); });

  simgrid::s4u::this_actor::execute(1e9);
}

static void actor_b()
{
  simgrid::s4u::this_actor::execute(2e9);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]); /* - Load the platform description */

  /* Register a callback in the Actor::on_termination signal. It will be called for every terminated actors */
  simgrid::s4u::Actor::on_termination.connect(
      [](simgrid::s4u::Actor const& actor) { XBT_INFO("Actor %s terminates now", actor.get_cname()); });
  /* Register a callback in the Actor::on_destruction signal. It will be called for every destructed actors */
  simgrid::s4u::Actor::on_destruction.connect(
      [](simgrid::s4u::Actor const& actor) { XBT_INFO("Actor %s gets destroyed now", actor.get_cname()); });

  /* Create some actors */
  simgrid::s4u::Actor::create("A", simgrid::s4u::Host::by_name("Tremblay"), actor_a);
  simgrid::s4u::Actor::create("B", simgrid::s4u::Host::by_name("Fafard"), actor_b);

  e.run(); /* - Run the simulation */

  return 0;
}
