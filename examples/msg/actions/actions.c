/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "simix/simix.h"        /* semaphores for the barrier */
#include "xbt.h"                /* calloc, printf */
#include "simgrid_config.h"     /* getline */

XBT_LOG_NEW_DEFAULT_CATEGORY(actions,
                             "Messages specific for this msg example");
int communicator_size = 0;

typedef struct coll_ctr_t {
  int bcast_counter;
  int reduce_counter;
  int allReduce_counter;
} *coll_ctr;

/* Helper function */
static double parse_double(const char *string)
{
  double value;
  char *endptr;

  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROW1(unknown_error, 0, "%s is not a double", string);
  return value;
}


/* My actions */
static void action_send(xbt_dynar_t action)
{
  char *name = NULL;
  char to[250];
  char *size = xbt_dynar_get_as(action, 3, char *);
  double clock = MSG_get_clock();
  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),
          xbt_dynar_get_as(action, 2, char *));
  //  char *to =  xbt_dynar_get_as(action, 2, char *);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  DEBUG2("Entering Send: %s (size: %lg)", name, parse_double(size));
  MSG_task_send(MSG_task_create(name, 0, parse_double(size), NULL), to);
  VERB2("%s %f", name, MSG_get_clock() - clock);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}


static int spawned_send(int argc, char *argv[])
{
  DEBUG3("%s: Sending %s on %s", MSG_process_get_name(MSG_process_self()),
         argv[1], argv[0]);
  MSG_task_send(MSG_task_create(argv[0], 0, parse_double(argv[1]), NULL),
                argv[0]);
  return 0;
}

static void Isend(xbt_dynar_t action)
{
  char spawn_name[80];
  char to[250];
  //  char *to = xbt_dynar_get_as(action, 2, char *);
  char *size = xbt_dynar_get_as(action, 3, char *);
  char **myargv;
  m_process_t comm_helper;
  double clock = MSG_get_clock();
  DEBUG1("Isend on %s: spawn process ",
         MSG_process_get_name(MSG_process_self()));

  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),
          xbt_dynar_get_as(action, 2, char *));
  myargv = (char **) calloc(3, sizeof(char *));

  myargv[0] = xbt_strdup(to);
  myargv[1] = xbt_strdup(size);
  myargv[2] = NULL;

  //    sprintf(spawn_name,"%s_wait",MSG_process_get_name(MSG_process_self()));
  sprintf(spawn_name, "%s_wait", to);
  comm_helper =
      MSG_process_create_with_arguments(spawn_name, spawned_send,
                                        NULL, MSG_host_self(), 2, myargv);
  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);
}


static void action_recv(xbt_dynar_t action)
{
  char *name = NULL;
  char mailbox_name[250];
  m_task_t task = NULL;
  double clock = MSG_get_clock();
  //FIXME: argument of action ignored so far; semantic not clear
  //char *from=xbt_dynar_get_as(action,2,char*);
  sprintf(mailbox_name, "%s_%s", xbt_dynar_get_as(action, 2, char *),
          MSG_process_get_name(MSG_process_self()));

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  DEBUG1("Receiving: %s", name);
  MSG_task_receive(&task, mailbox_name);
  //  MSG_task_receive(&task, MSG_process_get_name(MSG_process_self()));
  VERB2("%s %f", name, MSG_get_clock() - clock);
  MSG_task_destroy(task);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static int spawned_recv(int argc, char *argv[])
{
  m_task_t task = NULL;
  DEBUG1("Receiving on %s", argv[0]);
  MSG_task_receive(&task, argv[0]);
  DEBUG1("Received %s", MSG_task_get_name(task));
  DEBUG1("waiter on %s", MSG_process_get_name(MSG_process_self()));
  MSG_task_send(MSG_task_create("waiter", 0, 0, NULL),
                MSG_process_get_name(MSG_process_self()));

  MSG_task_destroy(task);
  return 0;
}


static void Irecv(xbt_dynar_t action)
{
  char *name;
  m_process_t comm_helper;
  char mailbox_name[250];
  char **myargv;
  double clock = MSG_get_clock();
  DEBUG1("Irecv on %s: spawn process ",
         MSG_process_get_name(MSG_process_self()));

  sprintf(mailbox_name, "%s_%s", xbt_dynar_get_as(action, 2, char *),
          MSG_process_get_name(MSG_process_self()));
  name = bprintf("%s_wait", MSG_process_get_name(MSG_process_self()));
  myargv = (char **) calloc(2, sizeof(char *));

  myargv[0] = xbt_strdup(mailbox_name);
  myargv[1] = NULL;
  comm_helper = MSG_process_create_with_arguments(name, spawned_recv,
                                                  NULL, MSG_host_self(),
                                                  1, myargv);

  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);

  free(name);
}


static void action_wait(xbt_dynar_t action)
{
  char *name = NULL;
  char task_name[80];
  m_task_t task = NULL;
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  DEBUG1("Entering %s", name);
  sprintf(task_name, "%s_wait", MSG_process_get_name(MSG_process_self()));
  DEBUG1("wait: %s", task_name);
  MSG_task_receive(&task, task_name);
  MSG_task_destroy(task);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

/* FIXME: that's a poor man's implementation: we should take the message exchanges into account */
smx_sem_t barrier_semaphore = NULL;
static void barrier(xbt_dynar_t action)
{
  char *name = NULL;

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  DEBUG1("Entering barrier: %s", name);
  if (barrier_semaphore == NULL)        // first arriving on the barrier
    barrier_semaphore = SIMIX_sem_init(0);

  if (SIMIX_sem_get_capacity(barrier_semaphore) == -communicator_size + 1) {    // last arriving
    SIMIX_sem_release_forever(barrier_semaphore);
    SIMIX_sem_destroy(barrier_semaphore);
    barrier_semaphore = NULL;
  } else {                      // not last
    SIMIX_sem_acquire(barrier_semaphore);
  }

  DEBUG1("Exiting barrier: %s", name);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);

}

static void reduce(xbt_dynar_t action)
{
  int i;
  char *name;
  char task_name[80];
  char spawn_name[80];
  char **myargv;
  char *comm_size = xbt_dynar_get_as(action, 2, char *);
  char *comp_size = xbt_dynar_get_as(action, 3, char *);
  m_process_t comm_helper = NULL;
  m_task_t task = NULL, comp_task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  coll_ctr counters = (coll_ctr) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
              ", can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  if (!counters) {
    DEBUG0("Initialize the counters");
    counters = (coll_ctr) calloc(1, sizeof(struct coll_ctr_t));
  }

  name = bprintf("reduce_%d", counters->reduce_counter++);

  if (!strcmp(process_name, "p0")) {
    DEBUG2("%s: %s is the Root", name, process_name);
    for (i = 1; i < communicator_size; i++) {
      sprintf(spawn_name, "%s_p%d_%s", name, i,
              MSG_process_get_name(MSG_process_self()));
      sprintf(task_name, "%s_wait", spawn_name);
      myargv = (char **) calloc(2, sizeof(char *));

      myargv[0] = xbt_strdup(spawn_name);
      myargv[1] = NULL;

      comm_helper =
          MSG_process_create_with_arguments(task_name, spawned_recv,
                                            NULL, MSG_host_self(),
                                            1, myargv);
    }

    for (i = 1; i < communicator_size; i++) {
      sprintf(task_name, "%s_p%d_p0_wait", name, i);
      MSG_task_receive(&task, task_name);
      MSG_task_destroy(task);
      task = NULL;
    }

    comp_task =
        MSG_task_create("reduce_comp", parse_double(comp_size), 0, NULL);
    DEBUG1("%s: computing 'reduce_comp'", name);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    DEBUG1("%s: computed", name);
  } else {
    DEBUG2("%s: %s sends", name, process_name);
    sprintf(task_name, "%s_%s_p0", name, process_name);
    DEBUG1("put on %s", task_name);
    MSG_task_send(MSG_task_create(name, 0, parse_double(comm_size), NULL),
                  task_name);
  }

  MSG_process_set_data(MSG_process_self(), (void *) counters);
  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);
  free(name);
}

static void bcast(xbt_dynar_t action)
{
  int i;
  char *name;
  const char *process_name;
  char task_name[80];
  char spawn_name[80];
  char **myargv;
  m_process_t comm_helper = NULL;
  m_task_t task = NULL;
  char *size = xbt_dynar_get_as(action, 2, char *);
  coll_ctr counters = (coll_ctr) MSG_process_get_data(MSG_process_self());
  double clock = MSG_get_clock();

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
              ", can't use collective operations");


  process_name = MSG_process_get_name(MSG_process_self());
  if (!counters) {
    DEBUG0("Initialize the counters");
    counters = (coll_ctr) calloc(1, sizeof(struct coll_ctr_t));
  }

  name = bprintf("bcast_%d", counters->bcast_counter++);
  if (!strcmp(process_name, "p0")) {
    DEBUG2("%s: %s is the Root", name, process_name);

    for (i = 1; i < communicator_size; i++) {
      myargv = (char **) calloc(3, sizeof(char *));
      myargv[0] = xbt_strdup(name);
      myargv[1] = xbt_strdup(size);
      myargv[2] = NULL;

      sprintf(spawn_name, "%s_%d", myargv[0], i);
      comm_helper =
          MSG_process_create_with_arguments(spawn_name, spawned_send,
                                            NULL, MSG_host_self(), 2,
                                            myargv);
    }

    for (i = 1; i < communicator_size; i++) {
      sprintf(task_name, "p%d_wait", i);
      DEBUG1("get on %s", task_name);
      MSG_task_receive(&task, task_name);
      MSG_task_destroy(task);
      task = NULL;
    }
    DEBUG2("%s: all messages sent by %s have been received",
           name, process_name);
  } else {
    DEBUG2("%s: %s receives", name, process_name);
    MSG_task_receive(&task, name);
    MSG_task_destroy(task);
    DEBUG2("%s: %s has received", name, process_name);
    sprintf(task_name, "%s_wait", process_name);
    DEBUG1("put on %s", task_name);
    MSG_task_send(MSG_task_create("waiter", 0, 0, NULL), task_name);
  }

  MSG_process_set_data(MSG_process_self(), (void *) counters);
  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);
  free(name);
}


static void action_sleep(xbt_dynar_t action)
{
  char *name = NULL;
  char *duration = xbt_dynar_get_as(action, 2, char *);
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  DEBUG1("Entering %s", name);
  MSG_process_sleep(parse_double(duration));
  VERB2("%s %f ", name, MSG_get_clock() - clock);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static void allReduce(xbt_dynar_t action)
{
  int i;
  char *name;
  char task_name[80];
  char spawn_name[80];
  char **myargv;
  char *comm_size = xbt_dynar_get_as(action, 2, char *);
  char *comp_size = xbt_dynar_get_as(action, 3, char *);
  m_process_t comm_helper = NULL;
  m_task_t task = NULL, comp_task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  coll_ctr counters = (coll_ctr) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
              ", can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  if (!counters) {
    DEBUG0("Initialize the counters");
    counters = (coll_ctr) calloc(1, sizeof(struct coll_ctr_t));
  }

  name = bprintf("allReduce_%d", counters->allReduce_counter++);

  if (!strcmp(process_name, "p0")) {
    DEBUG2("%s: %s is the Root", name, process_name);
    for (i = 1; i < communicator_size; i++) {
      sprintf(spawn_name, "%s_p%d_%s", name, i,
              MSG_process_get_name(MSG_process_self()));
      sprintf(task_name, "%s_wait", spawn_name);
      myargv = (char **) calloc(2, sizeof(char *));

      myargv[0] = xbt_strdup(spawn_name);
      myargv[1] = NULL;

      comm_helper =
          MSG_process_create_with_arguments(task_name, spawned_recv,
                                            NULL, MSG_host_self(),
                                            1, myargv);
    }

    for (i = 1; i < communicator_size; i++) {
      sprintf(task_name, "%s_p%d_p0_wait", name, i);
      MSG_task_receive(&task, task_name);
      MSG_task_destroy(task);
      task = NULL;
    }

    comp_task =
        MSG_task_create("allReduce_comp", parse_double(comp_size), 0,
                        NULL);
    DEBUG1("%s: computing 'reduce_comp'", name);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    DEBUG1("%s: computed", name);

    for (i = 1; i < communicator_size; i++) {
      myargv = (char **) calloc(3, sizeof(char *));
      myargv[0] = xbt_strdup(name);
      myargv[1] = xbt_strdup(comm_size);
      myargv[2] = NULL;

      sprintf(spawn_name, "%s_%d", myargv[0], i);
      comm_helper =
          MSG_process_create_with_arguments(spawn_name, spawned_send,
                                            NULL, MSG_host_self(), 2,
                                            myargv);
    }

    for (i = 1; i < communicator_size; i++) {
      sprintf(task_name, "p%d_wait", i);
      DEBUG1("get on %s", task_name);
      MSG_task_receive(&task, task_name);
      MSG_task_destroy(task);
      task = NULL;
    }
    DEBUG2("%s: all messages sent by %s have been received",
           name, process_name);

  } else {
    DEBUG2("%s: %s sends", name, process_name);
    sprintf(task_name, "%s_%s_p0", name, process_name);
    DEBUG1("put on %s", task_name);
    MSG_task_send(MSG_task_create(name, 0, parse_double(comm_size), NULL),
                  task_name);

    MSG_task_receive(&task, name);
    MSG_task_destroy(task);
    DEBUG2("%s: %s has received", name, process_name);
    sprintf(task_name, "%s_wait", process_name);
    DEBUG1("put on %s", task_name);
    MSG_task_send(MSG_task_create("waiter", 0, 0, NULL), task_name);

  }

  MSG_process_set_data(MSG_process_self(), (void *) counters);
  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);
  free(name);
}

static void comm_size(xbt_dynar_t action)
{
  char *name = NULL;
  char *size = xbt_dynar_get_as(action, 2, char *);
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");
  communicator_size = parse_double(size);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static void compute(xbt_dynar_t action)
{
  char *name = NULL;
  char *amout = xbt_dynar_get_as(action, 2, char *);
  m_task_t task = MSG_task_create(name, parse_double(amout), 0, NULL);
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");
  DEBUG1("Entering %s", name);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  /* Check the given arguments */
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file [action_files]\n",
           argv[0]);
    printf
        ("example: %s msg_platform.xml msg_deployment.xml actions # if all actions are in the same file\n",
         argv[0]);
    printf
        ("example: %s msg_platform.xml msg_deployment.xml # if actions are in separate files, specified in deployment\n",
         argv[0]);
    exit(1);
  }

  /*  Simulation setting */
  MSG_create_environment(argv[1]);

  /* No need to register functions as in classical MSG programs: the actions get started anyway */
  MSG_launch_application(argv[2]);

  /*   Action registration */
  MSG_action_register("comm_size", comm_size);
  MSG_action_register("send", action_send);
  MSG_action_register("Isend", Isend);
  MSG_action_register("recv", action_recv);
  MSG_action_register("Irecv", Irecv);
  MSG_action_register("wait", action_wait);
  MSG_action_register("barrier", barrier);
  MSG_action_register("bcast", bcast);
  MSG_action_register("reduce", reduce);
  MSG_action_register("allReduce", allReduce);
  MSG_action_register("sleep", action_sleep);
  MSG_action_register("compute", compute);


  /* Actually do the simulation using MSG_action_trace_run */
  res = MSG_action_trace_run(argv[3]);  // it's ok to pass a NULL argument here

  INFO1("Simulation time %g", MSG_get_clock());
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
