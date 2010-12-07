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
#include "instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actions,
                             "Messages specific for this msg example");
int communicator_size = 0;

static void action_Isend(xbt_dynar_t action);

typedef struct  {
  int last_Irecv_sender_id;
  int bcast_counter;
  int reduce_counter;
  int allReduce_counter;
  xbt_dynar_t isends;
} s_process_globals_t, *process_globals_t;

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

static int get_rank (const char *process_name)
{
  return atoi(&(process_name[1]));
} 

static void asynchronous_cleanup(void) {
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  /* Destroy any isend which correspond to completed communications */
  int found;
  msg_comm_t comm;
  while ((found = MSG_comm_testany(globals->isends)) != -1) {
    xbt_dynar_remove_at(globals->isends,found,&comm);
    MSG_comm_destroy(comm);
  }
}

/* My actions */
static int spawned_send(int argc, char *argv[])
{
  DEBUG3("%s: Sending %s on %s", MSG_process_get_name(MSG_process_self()),
         argv[1], argv[0]);
  MSG_task_send(MSG_task_create(argv[0], 0, parse_double(argv[1]), NULL),
                argv[0]);
  return 0;
}

static void action_send(xbt_dynar_t action)
{
  char *name = NULL;
  char to[250];
  char *size_str = xbt_dynar_get_as(action, 3, char *);
  double size=parse_double(size_str);
  double clock = MSG_get_clock(); /* this "call" is free thanks to inlining */

  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),
          xbt_dynar_get_as(action, 2, char *));
  //  char *to =  xbt_dynar_get_as(action, 2, char *);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

#ifdef HAVE_TRACING
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  int dst_traced = get_rank(xbt_dynar_get_as(action, 2, char *));
  TRACE_smpi_ptp_in(rank, rank, dst_traced, "send");
  TRACE_smpi_send(rank, rank, dst_traced);
#endif

  DEBUG2("Entering Send: %s (size: %lg)", name, size);
   if (size<65536) {
     action_Isend(action);
   } else {
     MSG_task_send(MSG_task_create(name, 0, size, NULL), to);
   }
   
   VERB2("%s %f", name, MSG_get_clock() - clock);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);

#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, rank, dst_traced, "send");
#endif

  asynchronous_cleanup();
}

static void action_Isend(xbt_dynar_t action)
{
  char to[250];
  //  char *to = xbt_dynar_get_as(action, 2, char *);
  char *size = xbt_dynar_get_as(action, 3, char *);
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());


  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),
          xbt_dynar_get_as(action, 2, char *));

  msg_comm_t comm =
      MSG_task_isend( MSG_task_create(to,0,parse_double(size),NULL), to);
  xbt_dynar_push(globals->isends,&comm);

  DEBUG1("Isend on %s", MSG_process_get_name(MSG_process_self()));
  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);

  asynchronous_cleanup();
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

#ifdef HAVE_TRACING
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  int src_traced = get_rank(xbt_dynar_get_as(action, 2, char *));
  TRACE_smpi_ptp_in(rank, src_traced, rank, "recv");
#endif

  DEBUG1("Receiving: %s", name);
  MSG_task_receive(&task, mailbox_name);
  //  MSG_task_receive(&task, MSG_process_get_name(MSG_process_self()));
  VERB2("%s %f", name, MSG_get_clock() - clock);
  MSG_task_destroy(task);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, "recv");
  TRACE_smpi_recv(rank, src_traced, rank);
#endif

  asynchronous_cleanup();
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


static void action_Irecv(xbt_dynar_t action)
{
  char *name;
  m_process_t comm_helper;
  char mailbox_name[250];
  char **myargv;
  double clock = MSG_get_clock();

  DEBUG1("Irecv on %s: spawn process ",
         MSG_process_get_name(MSG_process_self()));
#ifdef HAVE_TRACING
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  int src_traced = get_rank(xbt_dynar_get_as(action, 2, char *));
  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());
  counters->last_Irecv_sender_id = src_traced;
  MSG_process_set_data(MSG_process_self(), (void *) counters);

  TRACE_smpi_ptp_in(rank, src_traced, rank, "Irecv");
#endif

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
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, "Irecv");
#endif

  asynchronous_cleanup();
}


static void action_wait(xbt_dynar_t action)
{
  char *name = NULL;
  char task_name[80];
  m_task_t task = NULL;
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");
#ifdef HAVE_TRACING
  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());
  int src_traced = counters->last_Irecv_sender_id;
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  TRACE_smpi_ptp_in(rank, src_traced, rank, "wait");
#endif

  DEBUG1("Entering %s", name);
  sprintf(task_name, "%s_wait", MSG_process_get_name(MSG_process_self()));
  DEBUG1("wait: %s", task_name);
  MSG_task_receive(&task, task_name);
  MSG_task_destroy(task);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, "wait");
  TRACE_smpi_recv(rank, src_traced, rank);
#endif

}

/* FIXME: that's a poor man's implementation: we should take the message exchanges into account */
static void action_barrier(xbt_dynar_t action)
{
  char *name = NULL;
  static smx_mutex_t mutex = NULL;
  static smx_cond_t cond = NULL;
  static int processes_arrived_sofar=0;

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join(action, " ");

  if (mutex == NULL) {       // first arriving on the barrier
    mutex = SIMIX_req_mutex_init();
    cond = SIMIX_req_cond_init();
    processes_arrived_sofar=0;
  }
  DEBUG2("Entering barrier: %s (%d already there)", name,processes_arrived_sofar);

  SIMIX_req_mutex_lock(mutex);
  if (++processes_arrived_sofar == communicator_size) {
    SIMIX_req_cond_broadcast(cond);
    SIMIX_req_mutex_unlock(mutex);
  } else {
    SIMIX_req_cond_wait(cond,mutex);
    SIMIX_req_mutex_unlock(mutex);
  }

  DEBUG1("Exiting barrier: %s", name);

  processes_arrived_sofar--;
  if (!processes_arrived_sofar) {
    SIMIX_req_cond_destroy(cond);
    SIMIX_req_mutex_destroy(mutex);
    mutex=NULL;
  }

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);

}

static void action_reduce(xbt_dynar_t action)
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

  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
              ", can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

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

static void action_bcast(xbt_dynar_t action)
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
  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());
  double clock = MSG_get_clock();

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
              ", can't use collective operations");


  process_name = MSG_process_get_name(MSG_process_self());

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

static void action_allReduce(xbt_dynar_t action) {
  int i;
  char *allreduce_identifier;
  char mailbox[80];
  double comm_size = parse_double(xbt_dynar_get_as(action, 2, char *));
  double comp_size = parse_double(xbt_dynar_get_as(action, 3, char *));
  m_task_t task = NULL, comp_task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined, "
              "can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  allreduce_identifier = bprintf("allReduce_%d", counters->allReduce_counter++);

  if (!strcmp(process_name, "p0")) {
    DEBUG2("%s: %s is the Root", allreduce_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t,communicator_size-1);
    m_task_t *tasks = xbt_new0(m_task_t,communicator_size-1);
    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p%d_p0", allreduce_identifier, i);
      comms[i-1] = MSG_task_irecv(&(tasks[i-1]),mailbox);
    }
    MSG_comm_waitall(comms,communicator_size-1,-1);
    for (i = 1; i < communicator_size; i++) {
      // MSG_comm_destroy(comms[i-1]);
      MSG_task_destroy(tasks[i-1]);
    }
    free(tasks);

    comp_task = MSG_task_create("allReduce_comp", comp_size, 0, NULL);
    DEBUG1("%s: computing 'reduce_comp'", allreduce_identifier);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    DEBUG1("%s: computed", allreduce_identifier);

    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p0_p%d", allreduce_identifier, i);
      comms[i-1] =
          MSG_task_isend(MSG_task_create(mailbox,0,comm_size,NULL),
              mailbox);
    }
    MSG_comm_waitall(comms,communicator_size-1,-1);
    /* for (i = 1; i < communicator_size; i++) */
    /*   MSG_comm_destroy(comms[i-1]); */
    free(comms);

    DEBUG2("%s: all messages sent by %s have been received",
           allreduce_identifier, process_name);

  } else {
    DEBUG2("%s: %s sends", allreduce_identifier, process_name);
    sprintf(mailbox, "%s_%s_p0", allreduce_identifier, process_name);
    DEBUG1("put on %s", mailbox);
    MSG_task_send(MSG_task_create(allreduce_identifier, 0, comm_size, NULL),
                  mailbox);

    sprintf(mailbox, "%s_p0_%s", allreduce_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    DEBUG2("%s: %s has received", allreduce_identifier, process_name);
  }

  VERB2("%s %f", xbt_str_join(action, " "), MSG_get_clock() - clock);
  free(allreduce_identifier);
}

static void action_comm_size(xbt_dynar_t action)
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

static void action_compute(xbt_dynar_t action)
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

static void action_init(xbt_dynar_t action)
{ 
#ifdef HAVE_TRACING
  TRACE_smpi_init(get_rank(MSG_process_get_name(MSG_process_self())));
#endif
  DEBUG0("Initialize the counters");
  process_globals_t globals = (process_globals_t) calloc(1, sizeof(s_process_globals_t));
  globals->isends = xbt_dynar_new(sizeof(msg_comm_t),NULL);
  MSG_process_set_data(MSG_process_self(),globals);

}

static void action_finalize(xbt_dynar_t action)
{
#ifdef HAVE_TRACING
  TRACE_smpi_finalize(get_rank(MSG_process_get_name(MSG_process_self())));
#endif
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());
  if (globals){
    xbt_dynar_free_container(&(globals->isends));
    free(globals);
  }
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
  MSG_action_register("init",     action_init);
  MSG_action_register("finalize", action_finalize);
  MSG_action_register("comm_size",action_comm_size);
  MSG_action_register("send",     action_send);
  MSG_action_register("Isend",    action_Isend);
  MSG_action_register("recv",     action_recv);
  MSG_action_register("Irecv",    action_Irecv);
  MSG_action_register("wait",     action_wait);
  MSG_action_register("barrier",  action_barrier);
  MSG_action_register("bcast",    action_bcast);
  MSG_action_register("reduce",   action_reduce);
  MSG_action_register("allReduce",action_allReduce);
  MSG_action_register("sleep",    action_sleep);
  MSG_action_register("compute",  action_compute);


  /* Actually do the simulation using MSG_action_trace_run */
  res = MSG_action_trace_run(argv[3]);  // it's ok to pass a NULL argument here

  INFO1("Simulation time %g", MSG_get_clock());
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
