/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_app_token_ring, "Messages specific for this msg example");

/* Main function of all processes used in this example */
static int relay_runner(int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  xbt_assert(argc == 0, "The relay_runner function does not accept any parameter from the XML deployment file");
  int rank = xbt_str_parse_int(MSG_process_get_name(MSG_process_self()),
                               "Any process of this example must have a numerical name, not %s");
  char mailbox[256];

  if (rank == 0) {
    /* The root process (rank 0) first sends the token then waits to receive it back */
    snprintf(mailbox, 255, "%d", rank + 1);
    unsigned int task_comm_size = 1000000; /* The token is 1MB long*/
    msg_task_t task             = MSG_task_create("Token", 0, task_comm_size, NULL);
    XBT_INFO("Host \"%d\" send '%s' to Host \"%s\"", rank, MSG_task_get_name(task), mailbox);
    MSG_task_send(task, mailbox);
    task    = NULL;
    int res = MSG_task_receive(&task, MSG_process_get_name(MSG_process_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Host \"%d\" received \"%s\"", rank, MSG_task_get_name(task));
    MSG_task_destroy(task);

  } else {
    /* The others processes receive from their left neighbor (rank-1) and send to their right neighbor (rank+1) */
    msg_task_t task = NULL;
    int res         = MSG_task_receive(&task, MSG_process_get_name(MSG_process_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Host \"%d\" received \"%s\"", rank, MSG_task_get_name(task));

    if (rank + 1 == MSG_get_host_number())
      /* But the last process, which sends the token back to rank 0 */
      snprintf(mailbox, 255, "0");
    else
      snprintf(mailbox, 255, "%d", rank + 1);
    XBT_INFO("Host \"%d\" send '%s' to Host \"%s\"", rank, MSG_task_get_name(task), mailbox);
    MSG_task_send(task, mailbox);
  }
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  MSG_create_environment(argv[1]); /* - Load the platform description */
  xbt_dynar_t hosts = MSG_hosts_as_dynar();

  XBT_INFO("Number of hosts '%zu'", MSG_get_host_number());
  unsigned int i;
  msg_host_t h;
  xbt_dynar_foreach (hosts, i,
                     h) { /* - Give a unique rank to each host and create a @ref relay_runner process on each */
    char* name_host = bprintf("%u", i);
    MSG_process_create(name_host, relay_runner, NULL, h);
    free(name_host);
  }
  xbt_dynar_free(&hosts);

  int res = MSG_main(); /* - Run the simulation */
  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
