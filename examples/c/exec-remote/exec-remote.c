/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"

#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(exec_remote, "Messages specific for this example");

static void wizard(int argc, char* argv[])
{
  const_sg_host_t fafard = sg_host_by_name("Fafard");
  sg_host_t ginette      = sg_host_by_name("Ginette");
  sg_host_t boivin       = sg_host_by_name("Boivin");

  XBT_INFO("I'm a wizard! I can run a task on the Ginette host from the Fafard one! Look!");
  sg_exec_t exec = sg_actor_exec_init(48.492e6);
  sg_exec_set_host(exec, ginette);
  sg_exec_start(exec);
  XBT_INFO("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).");

  sg_actor_sleep_for(0.1);
  XBT_INFO("Loads in flops/s: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", sg_host_get_load(boivin),
           sg_host_get_load(fafard), sg_host_get_load(ginette));

  sg_exec_wait(exec);

  XBT_INFO("Done!");
  XBT_INFO("And now, harder. Start a remote task on Ginette and move it to Boivin after 0.5 sec");
  exec = sg_actor_exec_init(73293500);
  sg_exec_set_host(exec, ginette);
  sg_exec_start(exec);

  sg_actor_sleep_for(0.5);
  XBT_INFO("Loads before the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", sg_host_get_load(boivin),
           sg_host_get_load(fafard), sg_host_get_load(ginette));

  sg_exec_set_host(exec, boivin);

  sg_actor_sleep_for(0.1);
  XBT_INFO("Loads after the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", sg_host_get_load(boivin),
           sg_host_get_load(fafard), sg_host_get_load(ginette));

  sg_exec_wait(exec);
  XBT_INFO("Done!");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);
  sg_actor_create("test", sg_host_by_name("Fafard"), wizard, 0, NULL);

  simgrid_run();

  return 0;
}
