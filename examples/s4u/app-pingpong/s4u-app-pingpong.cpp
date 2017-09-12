/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_pingpong, "Messages specific for this s4u example");

static void pinger(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1, "The pinger function one argument from the XML deployment file");
  XBT_INFO("Ping -> %s", args[0].c_str());
  xbt_assert(simgrid::s4u::Host::by_name_or_null(args[0]) != nullptr, "Unknown host %s. Stopping Now! ",
             args[0].c_str());

  /* - Do the ping with a 1-Byte task (latency bound) ... */
  double* payload = new double();
  *payload        = simgrid::s4u::Engine::getClock();

  simgrid::s4u::Mailbox::byName(args[0])->put(payload, 1);
  /* - ... then wait for the (large) pong */
  double* sender_time =
      static_cast<double*>(simgrid::s4u::Mailbox::byName(simgrid::s4u::this_actor::getHost()->getName())->get());

  double communication_time = simgrid::s4u::Engine::getClock() - *sender_time;
  XBT_INFO("Task received : large communication (bandwidth bound)");
  XBT_INFO("Pong time (bandwidth bound): %.3f", communication_time);
  delete sender_time;
}

static void ponger(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1, "The ponger function one argument from the XML deployment file");
  XBT_INFO("Pong -> %s", args[0].c_str());
  xbt_assert(simgrid::s4u::Host::by_name_or_null(args[0]) != nullptr, "Unknown host %s. Stopping Now! ",
             args[0].c_str());

  /* - Receive the (small) ping first ....*/
  double* sender_time =
      static_cast<double*>(simgrid::s4u::Mailbox::byName(simgrid::s4u::this_actor::getHost()->getName())->get());
  double communication_time = simgrid::s4u::Engine::getClock() - *sender_time;
  XBT_INFO("Task received : small communication (latency bound)");
  XBT_INFO(" Ping time (latency bound) %f", communication_time);
  delete sender_time;

  /*  - ... Then send a 1GB pong back (bandwidth bound) */
  double* payload = new double();
  *payload        = simgrid::s4u::Engine::getClock();
  XBT_INFO("task_bw->data = %.3f", *payload);

  simgrid::s4u::Mailbox::byName(args[0])->put(payload, 1e9);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.loadPlatform(argv[1]);
  std::vector<std::string> args;
  args.push_back("Jupiter");
  simgrid::s4u::Actor::createActor("pinger", simgrid::s4u::Host::by_name("Tremblay"), pinger, args);

  args.pop_back();
  args.push_back("Tremblay");

  simgrid::s4u::Actor::createActor("ponger", simgrid::s4u::Host::by_name("Jupiter"), ponger, args);

  e.run();

  XBT_INFO("Total simulation time: %.3f", e.getClock());

  return 0;
}
