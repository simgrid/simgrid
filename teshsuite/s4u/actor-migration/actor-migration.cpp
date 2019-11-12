/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_migration, "Messages specific for this s4u example");

simgrid::s4u::Barrier* barrier;
static simgrid::s4u::ActorPtr controlled_process;

/* The Emigrant process will be moved from host to host. */
static void emigrant()
{
  XBT_INFO("I'll look for a new job on another machine ('Boivin') where the grass is greener.");
  simgrid::s4u::this_actor::set_host(
      simgrid::s4u::Host::by_name("Boivin")); /* - First, move to another host by myself */

  XBT_INFO("Yeah, found something to do");
  simgrid::s4u::this_actor::execute(98095000); /* - Execute some work there */
  simgrid::s4u::this_actor::sleep_for(2);
  XBT_INFO("Moving back home after work");
  simgrid::s4u::this_actor::set_host(simgrid::s4u::Host::by_name("Jacquelin")); /* - Move back to original location */
  simgrid::s4u::this_actor::set_host(simgrid::s4u::Host::by_name("Boivin")); /* - Go back to the other host to sleep*/
  simgrid::s4u::this_actor::sleep_for(4);
  controlled_process = simgrid::s4u::Actor::self(); /* - Get controlled at checkpoint */
  barrier->wait();
  simgrid::s4u::this_actor::suspend();
  simgrid::s4u::Host* h = simgrid::s4u::this_actor::get_host();
  XBT_INFO("I've been moved on this new host: %s", h->get_cname());
  XBT_INFO("Uh, nothing to do here. Stopping now");
}

/* The policeman check for emigrants and move them back to 'Jacquelin' */
static void policeman()
{
  XBT_INFO("Wait at the checkpoint."); /* - block on the mutex+condition */
  barrier->wait();
  controlled_process->set_host(simgrid::s4u::Host::by_name("Jacquelin")); /* - Move an emigrant to Jacquelin */
  XBT_INFO("I moved the emigrant");
  controlled_process->resume();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("emigrant", simgrid::s4u::Host::by_name("Jacquelin"), emigrant);
  simgrid::s4u::Actor::create("policeman", simgrid::s4u::Host::by_name("Boivin"), policeman);

  barrier = new simgrid::s4u::Barrier(2);
  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());
  delete barrier;

  return 0;
}
