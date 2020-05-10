/* Copyright (c) 2010-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/log.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_join, "Messages specific for this example");

static void sleeper(int argc, char* argv[])
{
  XBT_INFO("Sleeper started");
  sg_actor_sleep_for(3);
  XBT_INFO("I'm done. See you!");
}

static void master(int argc, char* argv[])
{
  sg_actor_t actor;

  XBT_INFO("Start sleeper");
  actor = sg_actor_create("sleeper from master", sg_host_self(), sleeper, 0, NULL);
  XBT_INFO("Join the sleeper (timeout 2)");
  sg_actor_join(actor, 2);

  XBT_INFO("Start sleeper");
  actor = sg_actor_create("sleeper from master", sg_host_self(), sleeper, 0, NULL);
  XBT_INFO("Join the sleeper (timeout 4)");
  sg_actor_join(actor, 4);

  XBT_INFO("Start sleeper");
  actor = sg_actor_create("sleeper from master", sg_host_self(), sleeper, 0, NULL);
  XBT_INFO("Join the sleeper (timeout 2)");
  sg_actor_join(actor, 2);

  XBT_INFO("Start sleeper");
  actor = sg_actor_create("sleeper from master", sg_host_self(), sleeper, 0, NULL);
  sg_actor_ref(actor); // We have to take that ref because the actor will stop before we join it
  XBT_INFO("Waiting 4");
  sg_actor_sleep_for(4);
  XBT_INFO("Join the sleeper after its end (timeout 1)");
  sg_actor_join(actor, 1);
  sg_actor_unref(actor); // Avoid to leak memory

  XBT_INFO("Goodbye now!");

  sg_actor_sleep_for(1);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("master", sg_host_by_name("Tremblay"), master, 0, NULL);

  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
