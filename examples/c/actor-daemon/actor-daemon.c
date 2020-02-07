/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_daemon, "Messages specific for this example");

/* The worker process, working for a while before leaving */
static void worker(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Let's do some work (for 10 sec on Boivin).");
  sg_actor_self_execute(980.95e6);
  XBT_INFO("I'm done now. I leave even if it makes the daemon die.");
}

/* The daemon, displaying a message every 3 seconds until all other processes stop */
static void my_daemon(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_actor_daemonize(sg_actor_self());

  while (1) {
    XBT_INFO("Hello from the infinite loop");
    sg_actor_sleep_for(3.0);
  }

  XBT_INFO("I will never reach that point: daemons are killed when regular processes are done");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  simgrid_load_platform(argv[1]);

  sg_actor_t w = sg_actor_init("worker", sg_host_by_name("Boivin"));
  sg_actor_start(w, worker, 0, NULL);
  sg_actor_t d = sg_actor_init("daemon", sg_host_by_name("Tremblay"));
  sg_actor_start(d, my_daemon, 0, NULL);

  simgrid_run();
}
