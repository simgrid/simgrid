/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  simgrid::s4u::this_actor::sleep_for(.5);
  XBT_INFO("Worker started (PID:%ld, PPID:%ld)", simgrid::s4u::this_actor::get_pid(),
           simgrid::s4u::this_actor::get_ppid());
  while (simgrid::s4u::this_actor::get_host()->is_on()) {
    simgrid::s4u::this_actor::yield();
    XBT_INFO("Plop i am not suspended");
    simgrid::s4u::this_actor::sleep_for(1);
  }
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  simgrid::s4u::this_actor::sleep_for(1);
  std::vector<simgrid::s4u::ActorPtr> actor_list = simgrid::s4u::this_actor::get_host()->get_all_actors();

  for (auto const& actor : actor_list) {
    XBT_INFO("Actor (pid=%ld, ppid=%ld, name=%s)", actor->get_pid(), actor->get_ppid(), actor->get_cname());
    if (simgrid::s4u::this_actor::get_pid() != actor->get_pid())
      actor->kill();
  }

  simgrid::s4u::ActorPtr actor =
      simgrid::s4u::Actor::create("worker from master", simgrid::s4u::this_actor::get_host(), worker);
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Suspend Actor (pid=%ld)", actor->get_pid());
  actor->suspend();

  XBT_INFO("Actor (pid=%ld) is %ssuspended", actor->get_pid(), actor->is_suspended() ? "" : "not ");
  simgrid::s4u::this_actor::sleep_for(2);

  XBT_INFO("Resume Actor (pid=%ld)", actor->get_pid());
  actor->resume();

  XBT_INFO("Actor (pid=%ld) is %ssuspended", actor->get_pid(), actor->is_suspended() ? "" : "not ");
  simgrid::s4u::this_actor::sleep_for(2);
  actor->kill();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), master);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Tremblay"), worker);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
