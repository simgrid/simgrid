/* Copyright (c) 2008-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "surf/surf_private.h"

int host(int argc, char *argv[]);
unsigned int task_comp_size = 50000000;
unsigned int task_comm_size = 1000000;

int nb_hosts; /* All declared hosts */

XBT_LOG_NEW_DEFAULT_CATEGORY(ring,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  @section MSG_ex_apps Examples of full applications
 * 
 * - <b>token_ring/ring_call.c</b>: Classical token ring
 *   communication, where a token is exchanged along a ring to reach
 *   every participant.
 * 
 */

int host(int argc, char *argv[])
{
  int host_number = atoi(MSG_process_get_name(MSG_process_self()));
  char mailbox[256];
  msg_task_t task = NULL;
  _XBT_GNUC_UNUSED int res;
  if (host_number == 0){ //master  send then receive
    sprintf(mailbox, "%d", host_number+1);
    task = MSG_task_create("Token", task_comp_size, task_comm_size, NULL);
    XBT_INFO("Host \"%d\" send '%s' to Host \"%s\"",host_number,task->name,mailbox);
    MSG_task_send(task, mailbox);
    task = NULL;
    res = MSG_task_receive(&(task), MSG_process_get_name(MSG_process_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Host \"%d\" received \"%s\"",host_number, MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  else{ //slave receive then send
    res = MSG_task_receive(&(task), MSG_process_get_name(MSG_process_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Host \"%d\" received \"%s\"",host_number, MSG_task_get_name(task));

    if(host_number+1 == nb_hosts)
      sprintf(mailbox, "0");
    else
      sprintf(mailbox, "%d", host_number+1);
    XBT_INFO("Host \"%d\" send '%s' to Host \"%s\"",host_number,task->name,mailbox);
    MSG_task_send(task, mailbox);
  }
  return 0;
}

int main(int argc, char **argv)
{
  int i,res;
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  nb_hosts =  xbt_dynar_length(hosts);
  MSG_function_register("host", host);

  XBT_INFO("Number of host '%d'",nb_hosts);
  for(i = 0 ; i<nb_hosts; i++)
  {
    char* name_host = bprintf("%d",i);
    MSG_process_create( name_host, host, NULL, xbt_dynar_get_as(hosts,i,msg_host_t) );
    free(name_host);
  }
  xbt_dynar_free(&hosts);

  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    return 0;
  else
    return 1;

}
