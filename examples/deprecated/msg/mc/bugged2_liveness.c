/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************************** Bugged2 ****************************************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* One client stay always in critical section                                 */
/* LTL property checked : !(GFcs)                                             */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/msg.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged3, "my log messages");

int cs = 0;

static int coordinator(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  int CS_used = 0;              // initially the CS is idle

  while (1) {
    msg_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      const char* req = MSG_task_get_data(task);
      if (CS_used) {
        XBT_INFO("CS already used.");
        msg_task_t answer = MSG_task_create("not grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
      } else {                  // can serve it immediately
        XBT_INFO("CS idle. Grant immediately");
        msg_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
        CS_used = 1;
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      XBT_INFO("CS release. resource now idle");
      CS_used = 0;
    }
    MSG_task_destroy(task);
    kind = NULL;
  }

  return 0;
}

static int client(int argc, char *argv[])
{
  xbt_assert(argc == 2);
  int my_pid = MSG_process_get_PID(MSG_process_self());
  char *my_mailbox = xbt_strdup(argv[1]);

  while(1){
    XBT_INFO("Client (%s) asks the request", my_mailbox);
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    msg_task_t answer = NULL;
    MSG_task_receive(&answer, my_mailbox);

    const char* kind = MSG_task_get_name(answer);

    if (!strcmp(kind, "grant")) {
      XBT_INFO("Client (%s) got the answer (grant). Sleep a bit and release it", my_mailbox);
      if(!strcmp(my_mailbox, "1"))
        cs = 1;
    }else{
      XBT_INFO("Client (%s) got the answer (not grant). Try again", my_mailbox);
    }

    MSG_task_destroy(answer);
    kind = NULL;

    MSG_process_sleep(my_pid);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MC_automaton_new_propositional_symbol_pointer("cs", &cs);

  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_bugged2_liveness.xml");
  MSG_main();

  return 0;

}
