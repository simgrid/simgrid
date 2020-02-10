/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/barrier.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_migrate, "Messages specific for this example");

static void worker(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_host_t first  = sg_host_by_name(argv[1]);
  sg_host_t second = sg_host_by_name(argv[2]);

  double flopAmount = sg_host_speed(first) * 5 + sg_host_speed(second) * 5;

  XBT_INFO("Let's move to %s to execute %.2f Mflops (5sec on %s and 5sec on %s)", argv[1], flopAmount / 1e6, argv[1],
           argv[2]);

  sg_actor_set_host(sg_actor_self(), first);
  sg_actor_self_execute(flopAmount);

  XBT_INFO("I wake up on %s. Let's suspend a bit", sg_host_get_name(sg_host_self()));

  sg_actor_suspend(sg_actor_self());

  XBT_INFO("I wake up on %s", sg_host_get_name(sg_host_self()));
  XBT_INFO("Done");
}

static void monitor(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_host_t jacquelin = sg_host_by_name("Jacquelin");
  sg_host_t fafard    = sg_host_by_name("Fafard");

  int actor_argc           = 3;
  const char* actor_argv[] = {"worker", "Boivin", "Jacquelin", NULL};
  sg_actor_t actor         = sg_actor_init("worker", sg_host_by_name("Fafard"));
  sg_actor_start(actor, worker, actor_argc, actor_argv);

  sg_actor_sleep_for(5);

  XBT_INFO("After 5 seconds, move the process to %s", sg_host_get_name(jacquelin));
  sg_actor_set_host(actor, jacquelin);

  sg_actor_sleep_until(15);
  XBT_INFO("At t=15, move the process to %s and resume it.", sg_host_get_name(fafard));
  sg_actor_set_host(actor, fafard);
  sg_actor_resume(actor);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */
  /* - Create and deploy the emigrant and policeman processes */
  sg_actor_t actor = sg_actor_init("monitor", sg_host_by_name("Boivin"));
  sg_actor_start(actor, monitor, 0, NULL);

  simgrid_run();

  return 0;
}
