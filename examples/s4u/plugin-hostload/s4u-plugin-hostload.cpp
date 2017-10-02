/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/load.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void execute_load_test()
{
  s4u_Host* host = simgrid::s4u::Host::by_name("MyHost1");

  XBT_INFO("Initial peak speed: %.0E flop/s; number of flops computed so far: %.0E (should be 0)", host->getSpeed(),
           sg_host_get_computed_flops(host));

  double start = simgrid::s4u::Engine::getClock();
  XBT_INFO("Sleep for 10 seconds");
  simgrid::s4u::this_actor::sleep_for(10);

  XBT_INFO("Done sleeping %.2fs; peak speed: %.0E flop/s; number of flops computed so far: %.0E (nothing should have "
           "changed)",
           simgrid::s4u::Engine::getClock() - start, host->getSpeed(), sg_host_get_computed_flops(host));

  // Run a task
  start = simgrid::s4u::Engine::getClock();
  XBT_INFO("Run a task of %.0E flops", 100E6);
  simgrid::s4u::this_actor::execute(100E6);

  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.0E",
           simgrid::s4u::Engine::getClock() - start, host->getSpeed(), sg_host_get_computed_flops(host));

  // ========= Change power peak =========
  int pstate = 2;
  host->setPstate(pstate);
  XBT_INFO("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           host->getPstateSpeed(pstate), host->getSpeed());

  // Run a second task
  start = simgrid::s4u::Engine::getClock();
  XBT_INFO("Run a task of %.0E flops", 100E6);
  simgrid::s4u::this_actor::execute(100E6);
  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.0E",
           simgrid::s4u::Engine::getClock() - start, host->getSpeed(), sg_host_get_computed_flops(host));

  start = simgrid::s4u::Engine::getClock();
  XBT_INFO("========= Requesting a reset of the computation counter");
  sg_host_load_reset(host);
  XBT_INFO("Sleep for 4 seconds");
  simgrid::s4u::this_actor::sleep_for(4);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           simgrid::s4u::Engine::getClock() - start, host->getSpeed(), sg_host_get_computed_flops(host));

  // =========== Turn the other host off ==========
  s4u_Host* host2 = simgrid::s4u::Host::by_name("MyHost2");
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed %.0f flops so far.",
           sg_host_get_computed_flops(host2));
  host2->turnOff();
  start = simgrid::s4u::Engine::getClock();
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           simgrid::s4u::Engine::getClock() - start, host->getSpeed(), sg_host_get_computed_flops(host));
}

int main(int argc, char* argv[])
{
  sg_host_load_plugin_init();
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("load_test", simgrid::s4u::Host::by_name("MyHost1"), execute_load_test);

  e.run();

  XBT_INFO("Total simulation time: %.2f", simgrid::s4u::Engine::getClock());

  return 0;
}
