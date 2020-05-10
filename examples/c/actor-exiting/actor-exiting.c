/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* In C, there is a single way of being informed when an actor exits.
 *
 * The sg_actor_on_exit() function allows one to register a function to be
 * executed when this very actor exits. The registered function will run
 * when this actor terminates (either because its main function returns, or
 * because it's killed in any way). No simcall are allowed here: your actor
 * is dead already, so it cannot interact with its environment in any way
 * (network, executions, disks, etc).
 *
 * Usually, the functions registered in sg_actor_on_exit() are in charge
 * of releasing any memory allocated by the actor during its execution.
 */

#include <simgrid/actor.h>
#include <simgrid/engine.h>
#include <simgrid/host.h>
#include <simgrid/mailbox.h>

#include <xbt/asserts.h>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_exiting, "Messages specific for this example");

static void A_on_exit(int ignored1, void* ignored2)
{
  XBT_INFO("I stop now");
}

static void actorA_fun(int argc, char* argv[])
{
  // Register a lambda function to be executed once it stops
  sg_actor_on_exit(&A_on_exit, NULL);

  sg_actor_sleep_for(1);
}
static void actorB_fun(int argc, char* argv[])
{
  sg_actor_sleep_for(2);
}
static void C_on_exit(int failed, void* ignored2)
{
  if (failed) {
    XBT_INFO("I was killed!");
    if (xbt_log_no_loc)
      XBT_INFO("The backtrace would be displayed here if --log=no_loc would not have been passed");
    else
      xbt_backtrace_display_current();
  } else
    XBT_INFO("Exiting gracefully.");
}
static void actorC_fun(int argc, char* argv[])
{
  // Register a lambda function to be executed once it stops
  sg_actor_on_exit(&C_on_exit, NULL);

  sg_actor_sleep_for(3);
  XBT_INFO("And now, induce a deadlock by waiting for a message that will never come\n\n");
  sg_mailbox_get(sg_mailbox_by_name("nobody"));
  xbt_die("Receiving is not supposed to succeed when nobody is sending");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */

  sg_actor_create("A", sg_host_by_name("Tremblay"), actorA_fun, 0, NULL);
  sg_actor_create("B", sg_host_by_name("Fafard"), actorB_fun, 0, NULL);
  sg_actor_create("C", sg_host_by_name("Ginette"), actorC_fun, 0, NULL);

  simgrid_run();

  return 0;
}
