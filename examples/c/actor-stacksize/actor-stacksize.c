/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This code tests that we can change the stack-size between the actors creation. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/config.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_stacksize, "Messages specific for this example");

static void actor(int argc, char* argv[])
{
  XBT_INFO("Hello");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);

  // If you don't specify anything, you get the default size (8Mb) or the one passed on the command line
  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);

  // You can use sg_cfg_set_int(key, value) to pass a size that will be parsed. That value will be used for any
  // subsequent actors
  sg_cfg_set_int("contexts/stack-size", 16384);
  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);
  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);

  sg_cfg_set_int("contexts/stack-size", 32 * 1024);
  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);
  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);

  // Or you can use set_stacksize() before starting the actor to modify only this one
  sg_actor_t a = sg_actor_init("actor", sg_host_by_name("Tremblay"));
  sg_actor_set_stacksize(a, 64 * 1024);
  sg_actor_start(a, actor, 0, NULL);

  sg_actor_create("actor", sg_host_by_name("Tremblay"), actor, 0, NULL);

  simgrid_run();
  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
