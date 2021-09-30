/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_daemon, "Messages specific for this example");

/* The worker actor, working for a while before leaving */
static void worker(int argc, char* argv[])
{
  XBT_INFO("Let's do some work (for 10 sec on Boivin).");
  sg_actor_execute(980.95e6);
  XBT_INFO("I'm done now. I leave even if it makes the daemon die.");
}

/* The daemon, displaying a message every 3 seconds until all other actors stop */
static void my_daemon(int argc, char* argv[])
{
  sg_actor_daemonize(sg_actor_self());

  while (1) {
    XBT_INFO("Hello from the infinite loop");
    sg_actor_sleep_for(3.0);
  }

  XBT_INFO("I will never reach that point: daemons are killed when regular actors are done");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  simgrid_load_platform(argv[1]);

  sg_actor_create("worker", sg_host_by_name("Boivin"), worker, 0, NULL);
  sg_actor_create("daemon", sg_host_by_name("Tremblay"), my_daemon, 0, NULL);

  simgrid_run();
}
