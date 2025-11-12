/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_kill, "Messages specific for this s4u example");

static void victimA_fun()
{
  sg4::this_actor::on_exit([](bool /*failed*/) { XBT_INFO("I have been killed!"); });
  XBT_INFO("Hello!");
  XBT_INFO("Suspending myself");
  sg4::this_actor::suspend();          /* - Start by suspending itself */
  XBT_INFO("OK, OK. Let's work");      /* - Then is resumed and start to execute some flops */
  sg4::this_actor::execute(1e9);
  XBT_INFO("Bye!"); /* - But will never reach the end of it */
}

static void victimB_fun()
{
  XBT_INFO("Terminate before being killed");
}

static void killer()
{
  auto e = simgrid::s4u::this_actor::get_engine();

  XBT_INFO("Hello!"); /* - First start a victim actor */
  sg4::ActorPtr victimA = e->host_by_name("Fafard")->add_actor("victim A", victimA_fun);
  sg4::ActorPtr victimB = e->host_by_name("Jupiter")->add_actor("victim B", victimB_fun);
  sg4::this_actor::sleep_for(10); /* - Wait for 10 seconds */

  XBT_INFO("Resume the victim A"); /* - Resume it from its suspended state */
  victimA->resume();
  sg4::this_actor::sleep_for(2);

  XBT_INFO("Kill the victim A"); /* - and then kill it */
  sg4::Actor::by_pid(victimA->get_pid())->kill(); // You can retrieve an actor from its PID (and then kill it)

  sg4::this_actor::sleep_for(1);

  XBT_INFO("Kill victimB, even if it's already dead"); /* that's a no-op, there is no zombies in SimGrid */
  victimB->kill(); // the actor is automatically garbage-collected after this last reference

  sg4::this_actor::sleep_for(1);

  XBT_INFO("Start a new actor, and kill it right away");
  sg4::ActorPtr victimC = e->host_by_name("Jupiter")->add_actor("victim C", victimA_fun);
  victimC->kill();

  sg4::this_actor::sleep_for(1);

  XBT_INFO("Killing everybody but myself");
  sg4::Actor::kill_all();

  XBT_INFO("OK, goodbye now. I commit a suicide.");
  sg4::this_actor::exit();

  XBT_INFO("This line never gets displayed: I'm already dead since the previous line.");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]); /* - Load the platform description */
  /* - Create and deploy killer actor, that will create the victim actors  */
  e.host_by_name("Tremblay")->add_actor("killer", killer);

  e.run(); /* - Run the simulation */

  return 0;
}
