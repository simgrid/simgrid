/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/simix.h"      /* semaphores for the barrier */
#include <xbt/replay.h>

/** @addtogroup MSG_examples
 *
 *  @section MSG_ex_actions Trace driven simulations
 * 
 *  The <b>actions/actions.c</b> example demonstrates how to run trace-driven simulations. It is very handy when you
 *  want to test an algorithm or protocol that does nothing unless it receives some events from outside. For example,
 *  a P2P protocol reacts to requests from the user, but does nothing if there is no such event.
 * 
 *  In such situations, SimGrid allows to write your protocol in your C file, and the events to react to in a separate
 *  text file. Declare a function handling each of the events that you want to accept in your trace files, register
 *  them using \ref xbt_replay_action_register in your main, and then use \ref MSG_action_trace_run to launch the
 *  simulation. You can either have one trace file containing all your events, or a file per simulated process. Check
 *  the tesh files in the example directory for details on how to do it.
 *
 *  This example uses this approach to replay MPI-like traces. It comes with a set of event handlers reproducing MPI
 *  events. This is somehow similar to SMPI, yet differently implemented. This code should probably be changed to use
 *  SMPI internals instead, but wasn't, so far.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(actions, "Messages specific for this msg example");
int communicator_size = 0;

static void action_Isend(const char *const *action);

typedef struct {
  int last_Irecv_sender_id;
  int bcast_counter;
  int reduce_counter;
  int allReduce_counter;
  xbt_dynar_t isends;           /* of msg_comm_t */
  /* Used to implement irecv+wait */
  xbt_dynar_t irecvs;           /* of msg_comm_t */
  xbt_dynar_t tasks;            /* of msg_task_t */
} s_process_globals_t, *process_globals_t;

/* Helper function */
static double parse_double(const char *string)
{
  double value;
  char *endptr;

  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a double", string);
  return value;
}

#define ACT_DEBUG(...) \
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {  \
    char *NAME = xbt_str_join_array(action, " ");              \
    XBT_DEBUG(__VA_ARGS__);                                    \
    xbt_free(NAME);                                            \
  } else ((void)0)

static void log_action(const char *const *action, double date)
{
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, date);
    xbt_free(name);
  }
}

static void asynchronous_cleanup(void)
{
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  /* Destroy any isend which correspond to completed communications */
  int found;
  msg_comm_t comm;
  while ((found = MSG_comm_testany(globals->isends)) != -1) {
    xbt_dynar_remove_at(globals->isends, found, &comm);
    MSG_comm_destroy(comm);
  }
}

/* My actions */
static void action_send(const char *const *action)
{
  char to[250];
  const char *size_str = action[3];
  double size = parse_double(size_str);
  double clock = MSG_get_clock();

  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()), action[2]);

  ACT_DEBUG("Entering Send: %s (size: %g)", NAME, size);
  if (size < 65536) {
    action_Isend(action);
  } else {
    MSG_task_send(MSG_task_create(to, 0, size, NULL), to);
  }

  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_Isend(const char *const *action)
{
  char to[250];
  const char *size = action[3];
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()), action[2]);
  msg_comm_t comm = MSG_task_isend(MSG_task_create(to, 0, parse_double(size), NULL), to);
  xbt_dynar_push(globals->isends, &comm);

  XBT_DEBUG("Isend on %s", MSG_process_get_name(MSG_process_self()));
  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_recv(const char *const *action)
{
  char mailbox_name[250];
  msg_task_t task = NULL;
  double clock = MSG_get_clock();

  sprintf(mailbox_name, "%s_%s", action[2], MSG_process_get_name(MSG_process_self()));

  ACT_DEBUG("Receiving: %s", NAME);
  msg_error_t res = MSG_task_receive(&task, mailbox_name);
  log_action(action, MSG_get_clock() - clock);

  if (res == MSG_OK) {
    MSG_task_destroy(task);
  }
  asynchronous_cleanup();
}

static void action_Irecv(const char *const *action)
{
  char mailbox[250];
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  XBT_DEBUG("Irecv on %s", MSG_process_get_name(MSG_process_self()));

  sprintf(mailbox, "%s_%s", action[2], MSG_process_get_name(MSG_process_self()));
  msg_task_t t = NULL;
  xbt_dynar_push(globals->tasks, &t);
  msg_comm_t c = MSG_task_irecv(xbt_dynar_get_ptr(globals->tasks, xbt_dynar_length(globals->tasks) - 1), mailbox);
  xbt_dynar_push(globals->irecvs, &c);

  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_wait(const char *const *action)
{
  msg_task_t task = NULL;
  msg_comm_t comm;
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert(xbt_dynar_length(globals->irecvs), "action wait not preceded by any irecv: %s",
             xbt_str_join_array(action, " "));

  ACT_DEBUG("Entering %s", NAME);
  comm = xbt_dynar_pop_as(globals->irecvs, msg_comm_t);
  MSG_comm_wait(comm, -1);
  task = xbt_dynar_pop_as(globals->tasks, msg_task_t);
  MSG_comm_destroy(comm);
  MSG_task_destroy(task);

  log_action(action, MSG_get_clock() - clock);
}

/* FIXME: that's a poor man's implementation: we should take the message exchanges into account */
static void action_barrier(const char *const *action)
{
  static smx_mutex_t mutex = NULL;
  static smx_cond_t cond = NULL;
  static int processes_arrived_sofar = 0;

  if (mutex == NULL) {          // first arriving on the barrier
    mutex = simcall_mutex_init();
    cond = simcall_cond_init();
    processes_arrived_sofar = 0;
  }
  ACT_DEBUG("Entering barrier: %s (%d already there)", NAME, processes_arrived_sofar);

  simcall_mutex_lock(mutex);
  if (++processes_arrived_sofar == communicator_size) {
    simcall_cond_broadcast(cond);
    simcall_mutex_unlock(mutex);
  } else {
    simcall_cond_wait(cond, mutex);
    simcall_mutex_unlock(mutex);
  }

  ACT_DEBUG("Exiting barrier: %s", NAME);

  processes_arrived_sofar--;
  if (!processes_arrived_sofar) {
    SIMIX_cond_destroy(cond);
    SIMIX_mutex_destroy(mutex);
    mutex = NULL;
  }
}

static void action_reduce(const char *const *action)
{
  int i;
  char *reduce_identifier;
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  msg_task_t comp_task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert(communicator_size, "Size of Communicator is not defined can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  reduce_identifier = bprintf("reduce_%d", counters->reduce_counter++);

  if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", reduce_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t, communicator_size - 1);
    msg_task_t *tasks = xbt_new0(msg_task_t, communicator_size - 1);
    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p%d_p0", reduce_identifier, i);
      comms[i - 1] = MSG_task_irecv(&(tasks[i - 1]), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++) {
      MSG_comm_destroy(comms[i - 1]);
      MSG_task_destroy(tasks[i - 1]);
    }
    xbt_free(comms);
    xbt_free(tasks);

    comp_task = MSG_task_create("reduce_comp", comp_size, 0, NULL);
    XBT_DEBUG("%s: computing 'reduce_comp'", reduce_identifier);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    XBT_DEBUG("%s: computed", reduce_identifier);
  } else {
    XBT_DEBUG("%s: %s sends", reduce_identifier, process_name);
    sprintf(mailbox, "%s_%s_p0", reduce_identifier, process_name);
    XBT_DEBUG("put on %s", mailbox);
    MSG_task_send(MSG_task_create(reduce_identifier, 0, comm_size, NULL), mailbox);
  }

  log_action(action, MSG_get_clock() - clock);
  xbt_free(reduce_identifier);
}

static void action_bcast(const char *const *action)
{
  int i;
  char *bcast_identifier;
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  msg_task_t task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert(communicator_size, "Size of Communicator is not defined, can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  bcast_identifier = bprintf("bcast_%d", counters->bcast_counter++);

  if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", bcast_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t, communicator_size - 1);

    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p0_p%d", bcast_identifier, i);
      comms[i - 1] = MSG_task_isend(MSG_task_create(mailbox, 0, comm_size, NULL), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++)
      MSG_comm_destroy(comms[i - 1]);
    xbt_free(comms);

    XBT_DEBUG("%s: all messages sent by %s have been received", bcast_identifier, process_name);
  } else {
    sprintf(mailbox, "%s_p0_%s", bcast_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    XBT_DEBUG("%s: %s has received", bcast_identifier, process_name);
  }

  log_action(action, MSG_get_clock() - clock);
  xbt_free(bcast_identifier);
}

static void action_sleep(const char *const *action)
{
  const char *duration = action[2];
  double clock = MSG_get_clock();

  ACT_DEBUG("Entering %s", NAME);
  MSG_process_sleep(parse_double(duration));
  log_action(action, MSG_get_clock() - clock);
}

static void action_allReduce(const char *const *action)
{
  int i;
  char *allreduce_identifier;
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  msg_task_t task = NULL, comp_task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert(communicator_size, "Size of Communicator is not defined, can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  allreduce_identifier = bprintf("allReduce_%d", counters->allReduce_counter++);

  if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", allreduce_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t, communicator_size - 1);
    msg_task_t *tasks = xbt_new0(msg_task_t, communicator_size - 1);
    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p%d_p0", allreduce_identifier, i);
      comms[i - 1] = MSG_task_irecv(&(tasks[i - 1]), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++) {
      MSG_comm_destroy(comms[i - 1]);
      MSG_task_destroy(tasks[i - 1]);
    }
    xbt_free(tasks);

    comp_task = MSG_task_create("allReduce_comp", comp_size, 0, NULL);
    XBT_DEBUG("%s: computing 'reduce_comp'", allreduce_identifier);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    XBT_DEBUG("%s: computed", allreduce_identifier);

    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p0_p%d", allreduce_identifier, i);
      comms[i - 1] = MSG_task_isend(MSG_task_create(mailbox, 0, comm_size, NULL), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++)
      MSG_comm_destroy(comms[i - 1]);
    xbt_free(comms);

    XBT_DEBUG("%s: all messages sent by %s have been received", allreduce_identifier, process_name);
  } else {
    XBT_DEBUG("%s: %s sends", allreduce_identifier, process_name);
    sprintf(mailbox, "%s_%s_p0", allreduce_identifier, process_name);
    XBT_DEBUG("put on %s", mailbox);
    MSG_task_send(MSG_task_create(allreduce_identifier, 0, comm_size, NULL), mailbox);

    sprintf(mailbox, "%s_p0_%s", allreduce_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    XBT_DEBUG("%s: %s has received", allreduce_identifier, process_name);
  }

  log_action(action, MSG_get_clock() - clock);
  xbt_free(allreduce_identifier);
}

static void action_comm_size(const char *const *action)
{
  const char *size = action[2];
  double clock = MSG_get_clock();

  communicator_size = parse_double(size);
  log_action(action, MSG_get_clock() - clock);
}

static void action_compute(const char *const *action)
{
  const char *amount = action[2];
  msg_task_t task = MSG_task_create("task", parse_double(amount), 0, NULL);
  double clock = MSG_get_clock();

  ACT_DEBUG("Entering %s", NAME);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  log_action(action, MSG_get_clock() - clock);
}

static void action_init(const char *const *action)
{
  XBT_DEBUG("Initialize the counters");
  process_globals_t globals = (process_globals_t) calloc(1, sizeof(s_process_globals_t));
  globals->isends = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  globals->irecvs = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  globals->tasks = xbt_dynar_new(sizeof(msg_task_t), NULL);
  MSG_process_set_data(MSG_process_self(), globals);
}

static void action_finalize(const char *const *action)
{
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());
  if (globals) {
    asynchronous_cleanup();
    xbt_dynar_free_container(&(globals->isends));
    xbt_dynar_free_container(&(globals->irecvs));
    xbt_dynar_free_container(&(globals->tasks));
    xbt_free(globals);
  }
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  /* Check the given arguments */
  MSG_init(&argc, argv);
  /* Explicit initialization of the action module is required now*/
  MSG_action_init();

  xbt_assert(argc > 2,
       "Usage: %s platform_file deployment_file [action_files]\n"
       "\t# if all actions are in the same file\n"
       "\tExample: %s msg_platform.xml msg_deployment.xml actions\n"
       "\t# if actions are in separate files, specified in deployment\n"
       "\tExample: %s msg_platform.xml msg_deployment.xml ",
       argv[0],argv[0],argv[0]);

  printf("WARNING: THIS BINARY IS KINDA DEPRECATED\n"
   "This example is still relevant if you want to learn about MSG-based trace replay, but if you want to simulate "
   "MPI-like traces, you should use the newer version that is in the examples/smpi/replay directory instead.\n");
   
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);

  /*   Action registration */
  xbt_replay_action_register("init", action_init);
  xbt_replay_action_register("finalize", action_finalize);
  xbt_replay_action_register("comm_size", action_comm_size);
  xbt_replay_action_register("send", action_send);
  xbt_replay_action_register("Isend", action_Isend);
  xbt_replay_action_register("recv", action_recv);
  xbt_replay_action_register("Irecv", action_Irecv);
  xbt_replay_action_register("wait", action_wait);
  xbt_replay_action_register("barrier", action_barrier);
  xbt_replay_action_register("bcast", action_bcast);
  xbt_replay_action_register("reduce", action_reduce);
  xbt_replay_action_register("allReduce", action_allReduce);
  xbt_replay_action_register("sleep", action_sleep);
  xbt_replay_action_register("compute", action_compute);

  /* Actually do the simulation using MSG_action_trace_run */
  res = MSG_action_trace_run(argv[3]);  // it's ok to pass a NULL argument here

  XBT_INFO("Simulation time %g", MSG_get_clock());

  /* Explicit finalization of the action module is required now*/
  MSG_action_exit();

  return res != MSG_OK;
}
