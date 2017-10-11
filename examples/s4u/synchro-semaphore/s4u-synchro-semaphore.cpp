/* Copyright (c) 2010-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include "simgrid/s4u.hpp"
 #include "xbt/str.h"
 #include "simgrid/plugins/load.h"
 #include "simgrid/simix.h"
 #include <cstdlib>
 #include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_semaphore_example, "Messages specific for this msg example");

smx_sem_t sem;

static int peer(std::vector<std::string> args){
  int i = 0;
  double wait_time;
  unsigned int ite = args.size();
  while(i < ite) {
    wait_time = std::stod(args[i]);
    i++;
    simgrid::s4u::this_actor::sleep_for(wait_time);
    XBT_INFO("Trying to acquire %d", i);
    simcall_sem_acquire(sem);
    XBT_INFO("Acquired %d", i);
    wait_time = std::stod(args[i]);
    i++;
    simgrid::s4u::this_actor::sleep_for(wait_time);
    XBT_INFO("Releasing %d", i);
    simcall_sem_release(sem);
    XBT_INFO("Released %d", i);
  }
  simgrid::s4u::this_actor::sleep_for(50);
  XBT_INFO("Done");

  return 0;
}

int main(int argc, char* argv[])
{

  sg_host_load_plugin_init();
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);
  e.loadPlatform(argv[1]);

  sem = simcall_sem_init(1);

  std::vector<std::string> aliceTimes(8);
  aliceTimes[0] = xbt_strdup("0");
  aliceTimes[1] = xbt_strdup("1");
  aliceTimes[2] = xbt_strdup("3");
  aliceTimes[3] = xbt_strdup("5");
  aliceTimes[4] = xbt_strdup("1");
  aliceTimes[5] = xbt_strdup("2");
  aliceTimes[6] = xbt_strdup("5");
  aliceTimes[7] = xbt_strdup("0");

  std::vector<std::string> bobTimes(8);
  bobTimes[0] = xbt_strdup("0.9");
  bobTimes[1] = xbt_strdup("1");
  bobTimes[2] = xbt_strdup("1");
  bobTimes[3] = xbt_strdup("2");
  bobTimes[4] = xbt_strdup("2");
  bobTimes[5] = xbt_strdup("0");
  bobTimes[6] = xbt_strdup("0");
  bobTimes[7] = xbt_strdup("5");

  simgrid::s4u::Actor::createActor("Alice", simgrid::s4u::Host::by_name("Fafard"), peer, aliceTimes);
  simgrid::s4u::Actor::createActor("Bob", simgrid::s4u::Host::by_name("Fafard"), peer, bobTimes);

  e.run();

  SIMIX_sem_destroy(sem);

  XBT_INFO("Finished\n");

  return 0;
}
