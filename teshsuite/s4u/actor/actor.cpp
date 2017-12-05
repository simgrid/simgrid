/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  simgrid::s4u::this_actor::sleep_for(.5);
  XBT_INFO("Worker started (PID:%lu, PPID:%lu)", simgrid::s4u::this_actor::getPid(),
           simgrid::s4u::this_actor::getPpid());
  while (1) {
    XBT_INFO("Plop i am %ssuspended", simgrid::s4u::this_actor::isSuspended() ? "" : "not ");
    simgrid::s4u::this_actor::sleep_for(1);
  }
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  simgrid::s4u::this_actor::sleep_for(1);
  std::vector<simgrid::s4u::ActorPtr>* actor_list = new std::vector<simgrid::s4u::ActorPtr>();
  simgrid::s4u::this_actor::getHost()->actorList(actor_list);

  for (auto const& actor : *actor_list) {
    XBT_INFO("Actor (pid=%lu, ppid=%lu, name=%s)", actor->getPid(), actor->getPpid(), actor->getCname());
    if (simgrid::s4u::this_actor::getPid() != actor->getPid())
      actor->kill();
  }

  simgrid::s4u::ActorPtr actor =
      simgrid::s4u::Actor::createActor("worker from master", simgrid::s4u::this_actor::getHost(), worker);
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Suspend Actor (pid=%lu)", actor->getPid());
  actor->suspend();

  XBT_INFO("Actor (pid=%lu) is %ssuspended", actor->getPid(), actor->isSuspended() ? "" : "not ");
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Resume Actor (pid=%lu)", actor->getPid());
  actor->resume();

  XBT_INFO("Actor (pid=%lu) is %ssuspended", actor->getPid(), actor->isSuspended() ? "" : "not ");
  simgrid::s4u::this_actor::sleep_for(2);
  actor->kill();

  delete actor_list;
  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("master", simgrid::s4u::Host::by_name("Tremblay"), master);
  simgrid::s4u::Actor::createActor("worker", simgrid::s4u::Host::by_name("Tremblay"), worker);

  e.run();
  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
