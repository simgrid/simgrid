/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  sg4::this_actor::sleep_for(.5);
  XBT_INFO("Worker started (PID:%ld, PPID:%ld)", sg4::this_actor::get_pid(),
           sg4::this_actor::get_ppid());
  while (sg4::this_actor::get_host()->is_on()) {
    sg4::this_actor::yield();
    XBT_INFO("Plop i am not suspended");
    sg4::this_actor::sleep_for(1);
  }
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  sg4::this_actor::sleep_for(1);
  std::vector<sg4::ActorPtr> actor_list = sg4::this_actor::get_host()->get_all_actors();

  for (auto const& actor : actor_list) {
    XBT_INFO("Actor (pid=%ld, ppid=%ld, name=%s)", actor->get_pid(), actor->get_ppid(), actor->get_cname());
    if (sg4::this_actor::get_pid() != actor->get_pid())
      actor->kill();
  }
  
  sg4::ActorPtr actor = sg4::this_actor::get_host()->add_actor("worker from master", worker);
  sg4::this_actor::sleep_for(2);

  XBT_INFO("Suspend Actor (pid=%ld)", actor->get_pid());
  actor->suspend();

  XBT_INFO("Actor (pid=%ld) is %ssuspended", actor->get_pid(), actor->is_suspended() ? "" : "not ");
  sg4::this_actor::sleep_for(2);

  XBT_INFO("Resume Actor (pid=%ld)", actor->get_pid());
  actor->resume();

  XBT_INFO("Actor (pid=%ld) is %ssuspended", actor->get_pid(), actor->is_suspended() ? "" : "not ");
  sg4::this_actor::sleep_for(2);
  actor->kill();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("master", master);
  e.host_by_name("Tremblay")->add_actor("worker", worker);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
