/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/msg.h>
#include <xbt/replay.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(actions, "Messages specific for this msg example");
int communicator_size = 0;

static void action_Isend(const char* const* action);

typedef struct {
  int last_Irecv_sender_id;
  int bcast_counter;
  xbt_dynar_t isends; /* of msg_comm_t */
  /* Used to implement irecv+wait */
  xbt_dynar_t irecvs; /* of msg_comm_t */
  xbt_dynar_t tasks;  /* of msg_task_t */
} s_process_globals_t;

typedef s_process_globals_t* process_globals_t;

/* Helper function */
static double parse_double(const char* string)
{
  double value;
  char* endptr;

  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a double", string);
  return value;
}

#define ACT_DEBUG(...)                                                                                                 \
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {                                                          \
    char* NAME = xbt_str_join_array(action, " ");                                                                      \
    XBT_DEBUG(__VA_ARGS__);                                                                                            \
    xbt_free(NAME);                                                                                                    \
  } else                                                                                                               \
  ((void)0)

static void log_action(const char* const* action, double date)
{
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {
    char* name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, date);
    xbt_free(name);
  }
}

static void asynchronous_cleanup(void)
{
  process_globals_t globals = (process_globals_t)MSG_process_get_data(MSG_process_self());

  /* Destroy any isend which correspond to completed communications */
  msg_comm_t comm;
  while (1 /*true*/) {
    int pos_found = MSG_comm_testany(globals->isends);
    if (pos_found == -1) /* none remaining */
      break;
    xbt_dynar_remove_at(globals->isends, pos_found, &comm);
    MSG_comm_destroy(comm);
  }
}

/* My actions */
static void action_send(const char* const* action)
{
  char to[250];
  const char* size_str = action[3];
  double size          = parse_double(size_str);
  double clock         = MSG_get_clock();

  snprintf(to, 249, "%s_%s", MSG_process_get_name(MSG_process_self()), action[2]);

  ACT_DEBUG("Entering Send: %s (size: %g)", NAME, size);
  if (size < 65536) {
    action_Isend(action);
  } else {
    MSG_task_send(MSG_task_create(to, 0, size, NULL), to);
  }

  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_Isend(const char* const* action)
{
  char to[250];
  const char* size          = action[3];
  double clock              = MSG_get_clock();
  process_globals_t globals = (process_globals_t)MSG_process_get_data(MSG_process_self());

  snprintf(to, 249, "%s_%s", MSG_process_get_name(MSG_process_self()), action[2]);
  msg_comm_t comm = MSG_task_isend(MSG_task_create(to, 0, parse_double(size), NULL), to);
  xbt_dynar_push(globals->isends, &comm);

  XBT_DEBUG("Isend on %s", MSG_process_get_name(MSG_process_self()));
  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_recv(const char* const* action)
{
  char mailbox_name[250];
  msg_task_t task = NULL;
  double clock    = MSG_get_clock();

  snprintf(mailbox_name, 249, "%s_%s", action[2], MSG_process_get_name(MSG_process_self()));

  ACT_DEBUG("Receiving: %s", NAME);
  msg_error_t res = MSG_task_receive(&task, mailbox_name);
  log_action(action, MSG_get_clock() - clock);

  if (res == MSG_OK) {
    MSG_task_destroy(task);
  }
  asynchronous_cleanup();
}

static void action_Irecv(const char* const* action)
{
  char mailbox[250];
  double clock              = MSG_get_clock();
  process_globals_t globals = (process_globals_t)MSG_process_get_data(MSG_process_self());

  XBT_DEBUG("Irecv on %s", MSG_process_get_name(MSG_process_self()));

  snprintf(mailbox, 249, "%s_%s", action[2], MSG_process_get_name(MSG_process_self()));
  msg_task_t t = NULL;
  xbt_dynar_push(globals->tasks, &t);
  msg_comm_t c = MSG_task_irecv(xbt_dynar_get_ptr(globals->tasks, xbt_dynar_length(globals->tasks) - 1), mailbox);
  xbt_dynar_push(globals->irecvs, &c);

  log_action(action, MSG_get_clock() - clock);
  asynchronous_cleanup();
}

static void action_wait(const char* const* action)
{
  msg_task_t task = NULL;
  msg_comm_t comm;
  double clock              = MSG_get_clock();
  process_globals_t globals = (process_globals_t)MSG_process_get_data(MSG_process_self());

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
static void action_barrier(const char* const* action)
{
  static msg_bar_t barrier           = NULL;
  static int processes_arrived_sofar = 0;

  if (barrier == NULL) {                                    // first arriving on the barrier
    msg_bar_t newbar = MSG_barrier_init(communicator_size); // This is a simcall, unscheduling the current process
    if (barrier == NULL)                                    // Still null, commit our new value
      barrier = newbar;
    else // some other process commited a new barrier before me, so dismiss mine
      MSG_barrier_destroy(newbar);
  }

  processes_arrived_sofar++;
  MSG_barrier_wait(barrier);

  ACT_DEBUG("Exiting barrier: %s", NAME);

  processes_arrived_sofar--;
  if (processes_arrived_sofar <= 0) {
    MSG_barrier_destroy(barrier);
    barrier = NULL;
  }
}

static void action_bcast(const char* const* action)
{
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  msg_task_t task  = NULL;
  double clock     = MSG_get_clock();

  process_globals_t counters = (process_globals_t)MSG_process_get_data(MSG_process_self());

  xbt_assert(communicator_size, "Size of Communicator is not defined, can't use collective operations");

  const char* process_name = MSG_process_get_name(MSG_process_self());

  char* bcast_identifier = bprintf("bcast_%d", counters->bcast_counter);
  counters->bcast_counter++;

  if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", bcast_identifier, process_name);

    msg_comm_t* comms = xbt_new0(msg_comm_t, communicator_size - 1);

    for (int i = 1; i < communicator_size; i++) {
      snprintf(mailbox, 79, "%s_p0_p%d", bcast_identifier, i);
      comms[i - 1] = MSG_task_isend(MSG_task_create(mailbox, 0, comm_size, NULL), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (int i = 1; i < communicator_size; i++)
      MSG_comm_destroy(comms[i - 1]);
    xbt_free(comms);

    XBT_DEBUG("%s: all messages sent by %s have been received", bcast_identifier, process_name);
  } else {
    snprintf(mailbox, 79, "%s_p0_%s", bcast_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    XBT_DEBUG("%s: %s has received", bcast_identifier, process_name);
  }

  log_action(action, MSG_get_clock() - clock);
  xbt_free(bcast_identifier);
}

static void action_comm_size(const char* const* action)
{
  const char* size = action[2];
  double clock     = MSG_get_clock();

  communicator_size = parse_double(size);
  log_action(action, MSG_get_clock() - clock);
}

static void action_compute(const char* const* action)
{
  const char* amount = action[2];
  msg_task_t task    = MSG_task_create("task", parse_double(amount), 0, NULL);
  double clock       = MSG_get_clock();

  ACT_DEBUG("Entering %s", NAME);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  log_action(action, MSG_get_clock() - clock);
}

static void action_init(const char* const* action)
{
  XBT_DEBUG("Initialize the counters");
  process_globals_t globals = (process_globals_t)calloc(1, sizeof(s_process_globals_t));
  globals->isends           = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  globals->irecvs           = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  globals->tasks            = xbt_dynar_new(sizeof(msg_task_t), NULL);
  MSG_process_set_data(MSG_process_self(), globals);
}

static void action_finalize(const char* const* action)
{
  process_globals_t globals = (process_globals_t)MSG_process_get_data(MSG_process_self());
  if (globals) {
    asynchronous_cleanup();
    xbt_dynar_free_container(&(globals->isends));
    xbt_dynar_free_container(&(globals->irecvs));
    xbt_dynar_free_container(&(globals->tasks));
    xbt_free(globals);
  }
}

int main(int argc, char* argv[])
{
  /* Check the given arguments */
  MSG_init(&argc, argv);
  /* Explicit initialization of the action module is required now*/
  MSG_action_init();

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file [action_files]\n"
                       "\t# if all actions are in the same file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml actions\n"
                       "\t# if actions are in separate files, specified in deployment\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml ",
             argv[0], argv[0], argv[0]);

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
  xbt_replay_action_register("compute", action_compute);

  /* Actually do the simulation using MSG_action_trace_run */
  msg_error_t res = MSG_action_trace_run(argv[3]); // it's ok to pass a NULL argument here

  XBT_INFO("Simulation time %g", MSG_get_clock());

  MSG_action_exit(); /* Explicit finalization of the action module */

  return res != MSG_OK;
}
