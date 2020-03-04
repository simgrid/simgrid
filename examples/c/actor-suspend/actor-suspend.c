/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_suspend, "Messages specific for this example");

/* The Lazy guy only wants to sleep, but can be awaken by the dream_master process. */
static void lazy_guy(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Nobody's watching me ? Let's go to sleep.");
  sg_actor_suspend(sg_actor_self()); /* - Start by suspending itself */
  XBT_INFO("Uuuh ? Did somebody call me ?");

  XBT_INFO("Going to sleep..."); /* - Then repetitively go to sleep, but got awaken */
  sg_actor_sleep_for(10.0);
  XBT_INFO("Mmm... waking up.");

  XBT_INFO("Going to sleep one more time (for 10 sec)...");
  sg_actor_sleep_for(10.0);
  XBT_INFO("Waking up once for all!");

  XBT_INFO("Ok, let's do some work, then (for 10 sec on Boivin).");
  sg_actor_execute(980.95e6);

  XBT_INFO("Mmmh, I'm done now. Goodbye.");
}

/* The Dream master: */
static void dream_master(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Let's create a lazy guy."); /* - Create a lazy_guy process */
  sg_actor_t lazy = sg_actor_create("Lazy", sg_host_self(), lazy_guy, 0, NULL);
  XBT_INFO("Let's wait a little bit...");
  sg_actor_sleep_for(10.0); /* - Wait for 10 seconds */
  XBT_INFO("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!");
  sg_actor_resume(lazy); /* - Then wake up the lazy_guy */

  sg_actor_sleep_for(5.0); /* Repeat two times: */
  XBT_INFO("Suspend the lazy guy while he's sleeping...");
  sg_actor_suspend(lazy); /* - Suspend the lazy_guy while he's asleep */
  XBT_INFO("Let him finish his siesta.");
  sg_actor_sleep_for(10.0); /* - Wait for 10 seconds */
  XBT_INFO("Wake up, lazy guy!");
  sg_actor_resume(lazy); /* - Then wake up the lazy_guy again */

  sg_actor_sleep_for(5.0);
  XBT_INFO("Suspend again the lazy guy while he's sleeping...");
  sg_actor_suspend(lazy);
  XBT_INFO("This time, don't let him finish his siesta.");
  sg_actor_sleep_for(2.0);
  XBT_INFO("Wake up, lazy guy!");
  sg_actor_resume(lazy);

  sg_actor_sleep_for(5.0);
  XBT_INFO("Give a 2 seconds break to the lazy guy while he's working...");
  sg_actor_suspend(lazy);
  sg_actor_sleep_for(2.0);
  XBT_INFO("Back to work, lazy guy!");
  sg_actor_resume(lazy);

  XBT_INFO("OK, I'm done here.");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);
  simgrid_register_function("dream_master", dream_master);

  sg_actor_create("dream_master", sg_host_by_name("Boivin"), dream_master, 0, NULL);

  simgrid_run();

  return 0;
}
