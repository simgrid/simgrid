/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* There is no bug on it, it is just provided to test the state space         */
/* reduction of DPOR.                                                         */
/******************************************************************************/

#include "simgrid/msg.h"

#define AMOUNT_OF_CLIENTS 4
#define CS_PER_PROCESS 2
XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");

static int coordinator(int argc, char *argv[])
{
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);   // dynamic vector storing requests (which are char*)
  int CS_used = 0;              // initially the CS is idle
  int todo = AMOUNT_OF_CLIENTS * CS_PER_PROCESS;        // amount of releases we are expecting
  while (todo > 0) {
    msg_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) {            // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request");
        xbt_dynar_push(requests, &req);
      } else {                  // can serve it immediatly
        XBT_INFO("CS idle. Grant immediatly");
        msg_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
        CS_used = 1;
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      if (!xbt_dynar_is_empty(requests)) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
        char *req;
        xbt_dynar_shift(requests, &req);
        MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
        todo--;
      } else {                  // nobody wants it
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
        todo--;
      }
    }
    MSG_task_destroy(task);
  }
  XBT_INFO("Received all releases, quit now");
  return 0;
}

static int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());
  // use my pid as name of mailbox to contact me
  char *my_mailbox = bprintf("%d", my_pid);
  // request the CS 3 times, sleeping a bit in between
  int i;
  for (i = 0; i < CS_PER_PROCESS; i++) {
    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");
    // wait the answer
    msg_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    MSG_task_destroy(grant);
    XBT_INFO("got the answer. Sleep a bit and release it");
    MSG_process_sleep(1);
    MSG_task_send(MSG_task_create("release", 0, 1000, NULL), "coordinator");
    MSG_process_sleep(my_pid);
  }
  XBT_INFO("Got all the CS I wanted, quit now");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_centralized_mutex.xml");
  MSG_main();
  return 0;
}
