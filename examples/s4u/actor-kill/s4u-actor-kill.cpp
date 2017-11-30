/* Copyright (c) 2017 The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_kill, "Messages specific for this s4u example");

static int on_exit(void*, void*)
{
  XBT_INFO("I have been killed!");
  return 0;
}

static void victimA_fun()
{
  simgrid::s4u::this_actor::onExit(on_exit, nullptr);
  XBT_INFO("Hello!");
  XBT_INFO("Suspending myself");
  simgrid::s4u::this_actor::suspend(); /* - Start by suspending itself */
  XBT_INFO("OK, OK. Let's work");      /* - Then is resumed and start to execute a task */
  simgrid::s4u::this_actor::execute(1e9);
  XBT_INFO("Bye!"); /* - But will never reach the end of it */
}

static void victimB_fun()
{
  XBT_INFO("Terminate before being killed");
}

static void killer()
{
  XBT_INFO("Hello!"); /* - First start a victim process */
  simgrid::s4u::ActorPtr victimA =
      simgrid::s4u::Actor::createActor("victim A", simgrid::s4u::Host::by_name("Fafard"), victimA_fun);
  simgrid::s4u::ActorPtr victimB =
      simgrid::s4u::Actor::createActor("victim B", simgrid::s4u::Host::by_name("Jupiter"), victimB_fun);
  simgrid::s4u::this_actor::sleep_for(10); /* - Wait for 10 seconds */

  XBT_INFO("Resume the victim A"); /* - Resume it from its suspended state */
  victimA->resume();
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Kill the victim A"); /* - and then kill it */
  victimA->kill();

  XBT_INFO("Kill victimB, even if it's already dead"); /* that's a no-op, there is no zombies in SimGrid */
  victimB->kill();
  simgrid::s4u::this_actor::sleep_for(1);

  XBT_INFO("Killing everybody but myself");
  simgrid::s4u::Actor::killAll();

  XBT_INFO("OK, goodbye now. I commit a suicide.");
  simgrid::s4u::this_actor::kill();

  XBT_INFO("This line will never get displayed: I'm already dead since the previous line.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  e.loadPlatform(argv[1]); /* - Load the platform description */
  /* - Create and deploy killer process, that will create the victim actors  */
  simgrid::s4u::Actor::createActor("killer", simgrid::s4u::Host::by_name("Tremblay"), killer);

  e.run(); /* - Run the simulation */

  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
