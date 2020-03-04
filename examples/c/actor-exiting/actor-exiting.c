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

#include <xbt/asserts.h>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_exiting, "Messages specific for this example");

static int my_on_exit(XBT_ATTRIB_UNUSED int ignored1, XBT_ATTRIB_UNUSED void* ignored2)
{
  XBT_INFO("I stop now");
  return 0;
}

static void actor_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  // Register a lambda function to be executed once it stops
  sg_actor_on_exit(&my_on_exit, NULL);

  sg_actor_execute(1e9);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */

  sg_actor_create("A", sg_host_by_name("Tremblay"), actor_fun, 0, NULL);

  simgrid_run();

  return 0;
}
