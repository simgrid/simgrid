/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"

#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <stddef.h>
#include <stdio.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(app_token_ring, "Messages specific for this msg example");

/* Main function of all actors used in this example */
static void relay_runner(int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  xbt_assert(argc == 0, "The relay_runner function does not accept any parameter from the XML deployment file");

  const char* name = sg_actor_self_get_name();
  int rank         = xbt_str_parse_int(name, "Any actor of this example must have a numerical name, not %s");

  sg_mailbox_t my_mailbox = sg_mailbox_by_name(name);

  /* The last actor sends the token back to rank 0, the others send to their right neighbor (rank+1) */
  char neighbor_mailbox_name[256];
  snprintf(neighbor_mailbox_name, 255, "%d", rank + 1 == sg_host_count() ? 0 : rank + 1);

  sg_mailbox_t neighbor_mailbox = sg_mailbox_by_name(neighbor_mailbox_name);

  char* res;
  if (rank == 0) {
    /* The root actor (rank 0) first sends the token then waits to receive it back */
    XBT_INFO("Host \"%d\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox_name);
    sg_mailbox_put(neighbor_mailbox, xbt_strdup("Token"), 1000000);

    res = (char*)sg_mailbox_get(my_mailbox);
    XBT_INFO("Host \"%d\" received \"%s\"", rank, res);
  } else {
    /* The others actors receive from their left neighbor (rank-1) and send to their right neighbor (rank+1) */
    res = (char*)sg_mailbox_get(my_mailbox);
    XBT_INFO("Host \"%d\" received \"%s\"", rank, res);
    XBT_INFO("Host \"%d\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox_name);
    sg_mailbox_put(neighbor_mailbox, xbt_strdup("Token"), 1000000);
  }
  free(res);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  simgrid_load_platform(argv[1]); /* - Load the platform description */

  size_t host_count = sg_host_count();
  sg_host_t* hosts  = sg_host_list();

  XBT_INFO("Number of hosts '%zu'", host_count);
  for (size_t i = 0; i < host_count; i++) {
    /* - Give a unique rank to each host and create a @ref relay_runner process on each */
    char* name_host  = bprintf("%zu", i);
    sg_actor_t actor = sg_actor_init(name_host, hosts[i]);
    sg_actor_start(actor, relay_runner, 0, NULL);
    free(name_host);
  }
  free(hosts);

  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());
  return 0;
}
