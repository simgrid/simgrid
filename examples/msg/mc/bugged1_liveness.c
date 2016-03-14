/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* Bug : CS requests of client 1 not satisfied                                      */
/* LTL property checked : G(r->F(cs)); (r=request of CS, cs=CS ok)            */
/******************************************************************************/

#ifdef GARBAGE_STACK
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "simgrid/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "bugged1_liveness.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged1_liveness, "my log messages");

int r=0; 
int cs=0;

#ifdef GARBAGE_STACK
/** Do not use a clean stack */
static void garbage_stack(void) {
  const size_t size = 256;
  int fd = open("/dev/urandom", O_RDONLY);
  char foo[size];
  read(fd, foo, size);
  close(fd);
}
#endif

static int coordinator(int argc, char *argv[])
{
  int CS_used = 0;   
  msg_task_t task = NULL, answer = NULL; 
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);
  char *req;

  while(1){  
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); 
    if (!strcmp(kind, "request")) {    
      req = MSG_task_get_data(task);
      if (CS_used) {           
        XBT_INFO("CS already used. Queue the request.");
        xbt_dynar_push(requests, &req);
      } else {               
        if(strcmp(req, "1") != 0){
          XBT_INFO("CS idle. Grant immediatly");
          answer = MSG_task_create("grant", 0, 1000, NULL);
          MSG_task_send(answer, req);
          CS_used = 1;
          answer = NULL;
        }
      }
    } else {
      if (!xbt_dynar_is_empty(requests)) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
        xbt_dynar_shift(requests, &req);
        if(strcmp(req, "1") != 0){
          MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
        }else{
          xbt_dynar_push(requests, &req);
          CS_used = 0;
        }
      }else{
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
      }
    }
    MSG_task_destroy(task);
    task = NULL;
    kind = NULL;
    req = NULL;
  }
  return 0;
}

static int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());

  char *my_mailbox = xbt_strdup(argv[1]);
  msg_task_t grant = NULL, release = NULL;

  while(1){
    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    if(strcmp(my_mailbox, "1") == 0){
      r = 1;
      cs = 0;
      XBT_INFO("Propositions changed : r=1, cs=0");
    }

    MSG_task_receive(&grant, my_mailbox);
    const char *kind = MSG_task_get_name(grant);

    if((strcmp(my_mailbox, "1") == 0) && (strcmp("grant", kind) == 0)){
      cs = 1;
      r = 0;
      XBT_INFO("Propositions changed : r=0, cs=1");
    }

    MSG_task_destroy(grant);
    grant = NULL;
    kind = NULL;

    XBT_INFO("%s got the answer. Sleep a bit and release it", argv[1]);

    MSG_process_sleep(1);

    release = MSG_task_create("release", 0, 1000, NULL);
    MSG_task_send(release, "coordinator");

    release = NULL;

    MSG_process_sleep(my_pid);

    if(strcmp(my_mailbox, "1") == 0){
      cs=0;
      r=0;
      XBT_INFO("Propositions changed : r=0, cs=0");
    }
    
  }
  return 0;
}

static int raw_client(int argc, char *argv[])
{
#ifdef GARBAGE_STACK
  // At this point the stack of the callee (client) is probably filled with
  // zeros and unitialized variables will contain 0. This call will place
  // random byes in the stack of the callee:
  garbage_stack();
#endif
  return client(argc, argv);
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MC_automaton_new_propositional_symbol_pointer("r", &r);
  MC_automaton_new_propositional_symbol_pointer("cs", &cs);

  MSG_create_environment(argv[1]);

  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", raw_client);
  MSG_launch_application(argv[2]);

  MSG_main();

  return 0;
}
