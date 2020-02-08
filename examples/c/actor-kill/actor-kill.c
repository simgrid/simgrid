/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"

#include "xbt/log.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_kill, "Messages specific for this example");

static int victim_on_exit(XBT_ATTRIB_UNUSED int ignored1, XBT_ATTRIB_UNUSED void* ignored2)
{
  XBT_INFO("I have been killed!");
  return 0;
}

static void victimA_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_actor_on_exit(&victim_on_exit, NULL);
  XBT_INFO("Hello!");
  XBT_INFO("Suspending myself");
  sg_actor_suspend(sg_actor_self()); /* - First suspend itself */
  XBT_INFO("OK, OK. Let's work");    /* - Then is resumed and start to execute a task */
  sg_actor_self_execute(1e9);
  XBT_INFO("Bye!"); /* - But will never reach the end of it */
}

static void victimB_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Terminate before being killed");
}

static void killer_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Hello!"); /* - First start a victim process */
  sg_actor_t victimA = sg_actor_init("victim A", sg_host_by_name("Fafard"));
  sg_actor_start(victimA, victimA_fun, 0, NULL);
  sg_actor_t victimB = sg_actor_init("victim B", sg_host_by_name("Jupiter"));
  sg_actor_start(victimB, victimB_fun, 0, NULL);
  sg_actor_sleep_for(10.0);

  XBT_INFO("Resume the victim A"); /* - Resume it from its suspended state */
  sg_actor_resume(victimA);
  sg_actor_sleep_for(2.0);

  XBT_INFO("Kill the victim A"); /* - and then kill it */
  sg_actor_kill(victimA);
  sg_actor_sleep_for(1.0);

  XBT_INFO("Kill victimB, even if it's already dead"); /* that's a no-op, there is no zombies in SimGrid */
  sg_actor_kill(victimB); // the actor is automatically garbage-collected after this last reference
  sg_actor_sleep_for(1.0);

  XBT_INFO("Start a new actor, and kill it right away");
  sg_actor_t victimC = sg_actor_init("victim C", sg_host_by_name("Jupiter"));
  sg_actor_start(victimC, victimA_fun, 0, NULL);
  sg_actor_kill(victimC);
  sg_actor_sleep_for(1.0);

  XBT_INFO("Killing everybody but myself");
  sg_actor_kill_all();

  XBT_INFO("OK, goodbye now. I commit a suicide.");
  sg_actor_exit();

  XBT_INFO("This line will never get displayed: I'm already dead since the previous line.");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  /* - Create and deploy killer process, that will create the victim process  */
  sg_actor_t killer = sg_actor_init("killer", sg_host_by_name("Tremblay"));
  sg_actor_start(killer, killer_fun, 0, NULL);

  simgrid_run();

  return 0;
}
