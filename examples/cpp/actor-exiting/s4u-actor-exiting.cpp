/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

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
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_exiting, "Messages specific for this s4u example");

static void actor_a()
{
  // Register a lambda function to be executed once it stops
  sg4::this_actor::on_exit([](bool /*failed*/) { XBT_INFO("I stop now"); });

  sg4::this_actor::sleep_for(1);
}

static void actor_b()
{
  sg4::this_actor::sleep_for(2);
}

static void actor_c()
{
  // Register a lambda function to be executed once it stops
  sg4::this_actor::on_exit([](bool failed) {
    if (failed) {
      XBT_INFO("I was killed!");
      if (xbt_log_no_loc)
        XBT_INFO("The backtrace would be displayed here if --log=no_loc would not have been passed");
      else
        xbt_backtrace_display_current();
    } else
      XBT_INFO("Exiting gracefully.");
  });

  sg4::this_actor::sleep_for(3);
  XBT_INFO("And now, induce a deadlock by waiting for a message that will never come\n\n");
  sg4::Mailbox::by_name("nobody")->get<void>();
  xbt_die("Receiving is not supposed to succeed when nobody is sending");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]); /* - Load the platform description */

  /* Register a callback in the Actor::on_termination signal. It will be called for every terminated actors */
  sg4::Actor::on_termination.connect(
      [](sg4::Actor const& actor) { XBT_INFO("Actor %s terminates now", actor.get_cname()); });
  /* Register a callback in the Actor::on_destruction signal. It will be called for every destructed actors */
  sg4::Actor::on_destruction.connect(
      [](sg4::Actor const& actor) { XBT_INFO("Actor %s gets destroyed now", actor.get_cname()); });

  /* Create some actors */
  sg4::Actor::create("A", sg4::Host::by_name("Tremblay"), actor_a);
  sg4::Actor::create("B", sg4::Host::by_name("Fafard"), actor_b);
  sg4::Actor::create("C", sg4::Host::by_name("Ginette"), actor_c);

  e.run(); /* - Run the simulation */

  return 0;
}
