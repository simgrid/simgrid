/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
#include <simgrid/s4u/Mutex.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_migration, "Messages specific for this s4u example");

simgrid::s4u::MutexPtr checkpoint                 = nullptr;
simgrid::s4u::ConditionVariablePtr identification = nullptr;
static simgrid::s4u::ActorPtr controlled_process  = nullptr;

/* The Emigrant process will be moved from host to host. */
static void emigrant()
{
  XBT_INFO("I'll look for a new job on another machine ('Boivin') where the grass is greener.");
  simgrid::s4u::this_actor::migrate(
      simgrid::s4u::Host::by_name("Boivin")); /* - First, move to another host by myself */

  XBT_INFO("Yeah, found something to do");
  simgrid::s4u::this_actor::execute(98095000);
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Moving back home after work");
  simgrid::s4u::this_actor::migrate(simgrid::s4u::Host::by_name("Jacquelin")); /* - Move back to original location */

  simgrid::s4u::this_actor::migrate(simgrid::s4u::Host::by_name("Boivin")); /* - Go back to the other host to sleep*/
  simgrid::s4u::this_actor::sleep_for(4);

  checkpoint->lock();                               /* - Get controlled at checkpoint */
  controlled_process = simgrid::s4u::Actor::self(); /* - and get moved back by the policeman process */
  identification->notify_all();
  checkpoint->unlock();

  simgrid::s4u::this_actor::suspend();

  XBT_INFO("I've been moved on this new host: %s", simgrid::s4u::this_actor::getHost()->getCname());
  XBT_INFO("Uh, nothing to do here. Stopping now");
}

/* The policeman check for emigrants and move them back to 'Jacquelin' */
static void policeman()
{
  checkpoint->lock();

  XBT_INFO("Wait at the checkpoint."); /* - block on the mutex+condition */
  while (controlled_process == nullptr)
    identification->wait(checkpoint);

  controlled_process->migrate(simgrid::s4u::Host::by_name("Jacquelin")); /* - Move an emigrant to Jacquelin */
  XBT_INFO("I moved the emigrant");
  controlled_process->resume();

  checkpoint->unlock();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);
  e.loadPlatform(argv[1]); /* - Load the platform description */

  /* - Create and deploy the emigrant and policeman processes */
  simgrid::s4u::Actor::createActor("emigrant", simgrid::s4u::Host::by_name("Jacquelin"), emigrant);
  simgrid::s4u::Actor::createActor("policeman", simgrid::s4u::Host::by_name("Boivin"), policeman);

  checkpoint     = simgrid::s4u::Mutex::createMutex(); /* - Initiate the mutex and conditions */
  identification = simgrid::s4u::ConditionVariable::createConditionVariable();
  e.run();

  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
