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
void SIMIX_ctx_raw_factory_init(smx_context_factory_t *factory);


XBT_LOG_NEW_DEFAULT_CATEGORY(actions,
                             "Messages specific for this msg example");
int communicator_size = 0;

static void action_Isend(char*const* action);

typedef struct  {
  int last_Irecv_sender_id;
  int bcast_counter;
  int reduce_counter;
  int allReduce_counter;
  xbt_dynar_t isends; /* of msg_comm_t */
  /* Used to implement irecv+wait */
  xbt_dynar_t irecvs; /* of msg_comm_t */
  xbt_dynar_t tasks; /* of m_task_t */
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
static void action_send(char*const* action)
{
  char *name = NULL;
  char to[250];
  const char *size_str = action[3];
  double size=parse_double(size_str);
  double clock = MSG_get_clock(); /* this "call" is free thanks to inlining */

  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),action[2]);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");

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

static void action_Isend(char*const* action)
{
  char to[250];
  const char *size = action[3];
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());


  sprintf(to, "%s_%s", MSG_process_get_name(MSG_process_self()),action[2]);
  msg_comm_t comm =
      MSG_task_isend( MSG_task_create(to,0,parse_double(size),NULL), to);
  xbt_dynar_push(globals->isends,&comm);

  DEBUG1("Isend on %s", MSG_process_get_name(MSG_process_self()));
  VERB2("%s %f", xbt_str_join_array(action, " "), MSG_get_clock() - clock);

  asynchronous_cleanup();
}


static void action_recv(char*const* action)
{
  char *name = NULL;
  char mailbox_name[250];
  m_task_t task = NULL;
  double clock = MSG_get_clock();

  sprintf(mailbox_name, "%s_%s", action[2],
          MSG_process_get_name(MSG_process_self()));

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");

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

static void action_Irecv(char*const* action)
{
  char mailbox[250];
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  DEBUG1("Irecv on %s", MSG_process_get_name(MSG_process_self()));
#ifdef HAVE_TRACING
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  int src_traced = get_rank(xbt_dynar_get_as(action, 2, char *));
  globals->last_Irecv_sender_id = src_traced;
  MSG_process_set_data(MSG_process_self(), (void *) globals);

  TRACE_smpi_ptp_in(rank, src_traced, rank, "Irecv");
#endif

  sprintf(mailbox, "%s_%s", action[2],
          MSG_process_get_name(MSG_process_self()));
  m_task_t t=NULL;
  xbt_dynar_push(globals->tasks,&t);
  msg_comm_t c =
      MSG_task_irecv(
          xbt_dynar_get_ptr(globals->tasks, xbt_dynar_length(globals->tasks)-1),
          mailbox);
  xbt_dynar_push(globals->irecvs,&c);

  VERB2("%s %f", xbt_str_join_array(action, " "), MSG_get_clock() - clock);

#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, "Irecv");
#endif

  asynchronous_cleanup();
}


static void action_wait(char*const* action)
{
  char *name = NULL;
  m_task_t task = NULL;
  msg_comm_t comm;
  double clock = MSG_get_clock();
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert1(xbt_dynar_length(globals->irecvs),
      "action wait not preceded by any irecv: %s", xbt_str_join_array(action," "));

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");
#ifdef HAVE_TRACING
  process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());
  int src_traced = counters->last_Irecv_sender_id;
  int rank = get_rank(MSG_process_get_name(MSG_process_self()));
  TRACE_smpi_ptp_in(rank, src_traced, rank, "wait");
#endif

  DEBUG1("Entering %s", name);
  comm = xbt_dynar_pop_as(globals->irecvs,msg_comm_t);
  MSG_comm_wait(comm,-1);
  task = xbt_dynar_pop_as(globals->tasks,m_task_t);
  MSG_comm_destroy(comm);
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
static void action_barrier(char*const* action)
{
  char *name = NULL;
  static smx_mutex_t mutex = NULL;
  static smx_cond_t cond = NULL;
  static int processes_arrived_sofar=0;

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");

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

static void action_reduce(char*const* action)
{
	int i;
	char *reduce_identifier;
	char mailbox[80];
	double comm_size = parse_double(action[2]);
	double comp_size = parse_double(action[3]);
	m_task_t comp_task = NULL;
	const char *process_name;
	double clock = MSG_get_clock();

	process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

	xbt_assert0(communicator_size, "Size of Communicator is not defined, "
			"can't use collective operations");

	process_name = MSG_process_get_name(MSG_process_self());

	reduce_identifier = bprintf("reduce_%d", counters->reduce_counter++);

	if (!strcmp(process_name, "p0")) {
		DEBUG2("%s: %s is the Root", reduce_identifier, process_name);

		msg_comm_t *comms = xbt_new0(msg_comm_t,communicator_size-1);
	    m_task_t *tasks = xbt_new0(m_task_t,communicator_size-1);
	    for (i = 1; i < communicator_size; i++) {
	      sprintf(mailbox, "%s_p%d_p0", reduce_identifier, i);
	      comms[i-1] = MSG_task_irecv(&(tasks[i-1]),mailbox);
	    }
	    MSG_comm_waitall(comms,communicator_size-1,-1);
	    for (i = 1; i < communicator_size; i++) {
	    	MSG_comm_destroy(comms[i-1]);
	    	MSG_task_destroy(tasks[i-1]);
	    }
	    free(tasks);

	    comp_task = MSG_task_create("reduce_comp", comp_size, 0, NULL);
	    DEBUG1("%s: computing 'reduce_comp'", reduce_identifier);
	    MSG_task_execute(comp_task);
	    MSG_task_destroy(comp_task);
	    DEBUG1("%s: computed", reduce_identifier);

	} else {
		DEBUG2("%s: %s sends", reduce_identifier, process_name);
		sprintf(mailbox, "%s_%s_p0", reduce_identifier, process_name);
	    DEBUG1("put on %s", mailbox);
	    MSG_task_send(MSG_task_create(reduce_identifier, 0, comm_size, NULL),
	                  mailbox);
	}

	VERB2("%s %f", xbt_str_join_array(action, " "), MSG_get_clock() - clock);
	free(reduce_identifier);
}

static void action_bcast(char*const* action)
{
	int i;
	char *bcast_identifier;
	char mailbox[80];
	double comm_size = parse_double(action[2]);
	m_task_t task = NULL;
	const char *process_name;
	double clock = MSG_get_clock();

	process_globals_t counters = (process_globals_t) MSG_process_get_data(MSG_process_self());

	xbt_assert0(communicator_size, "Size of Communicator is not defined, "
			"can't use collective operations");

	process_name = MSG_process_get_name(MSG_process_self());

	bcast_identifier = bprintf("bcast_%d", counters->bcast_counter++);

	if (!strcmp(process_name, "p0")) {
		DEBUG2("%s: %s is the Root", bcast_identifier, process_name);

	    msg_comm_t *comms = xbt_new0(msg_comm_t,communicator_size-1);

	    for (i = 1; i < communicator_size; i++) {
	      sprintf(mailbox, "%s_p0_p%d", bcast_identifier, i);
	      comms[i-1] =
	          MSG_task_isend(MSG_task_create(mailbox,0,comm_size,NULL),
	              mailbox);
	    }
	    MSG_comm_waitall(comms,communicator_size-1,-1);
		for (i = 1; i < communicator_size; i++)
	       MSG_comm_destroy(comms[i-1]);
	    free(comms);

	    DEBUG2("%s: all messages sent by %s have been received",
	           bcast_identifier, process_name);

	} else {
	    sprintf(mailbox, "%s_p0_%s", bcast_identifier, process_name);
	    MSG_task_receive(&task, mailbox);
	    MSG_task_destroy(task);
	    DEBUG2("%s: %s has received", bcast_identifier, process_name);
	}

	VERB2("%s %f", xbt_str_join_array(action, " "), MSG_get_clock() - clock);
	free(bcast_identifier);
}


static void action_sleep(char*const* action)
{
  char *name = NULL;
  const char *duration = action[2];
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");

  DEBUG1("Entering %s", name);
  MSG_process_sleep(parse_double(duration));
  VERB2("%s %f ", name, MSG_get_clock() - clock);

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static void action_allReduce(char*const* action) {
  int i;
  char *allreduce_identifier;
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
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
      MSG_comm_destroy(comms[i-1]);
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
    for (i = 1; i < communicator_size; i++)
       MSG_comm_destroy(comms[i-1]);
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

  VERB2("%s %f", xbt_str_join_array(action, " "), MSG_get_clock() - clock);
  free(allreduce_identifier);
}

static void action_comm_size(char*const* action)
{
  char *name = NULL;
  const char *size = action[2];
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");
  communicator_size = parse_double(size);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static void action_compute(char*const* action)
{
  char *name = NULL;
  const char *amout = action[2];
  m_task_t task = MSG_task_create(name, parse_double(amout), 0, NULL);
  double clock = MSG_get_clock();

  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    name = xbt_str_join_array(action, " ");
  DEBUG1("Entering %s", name);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  VERB2("%s %f", name, MSG_get_clock() - clock);
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose))
    free(name);
}

static void action_init(char*const* action)
{ 
#ifdef HAVE_TRACING
  TRACE_smpi_init(get_rank(MSG_process_get_name(MSG_process_self())));
#endif
  DEBUG0("Initialize the counters");
  process_globals_t globals = (process_globals_t) calloc(1, sizeof(s_process_globals_t));
  globals->isends = xbt_dynar_new(sizeof(msg_comm_t),NULL);
  globals->irecvs = xbt_dynar_new(sizeof(msg_comm_t),NULL);
  globals->tasks  = xbt_dynar_new(sizeof(m_task_t),NULL);
  MSG_process_set_data(MSG_process_self(),globals);

}

static void action_finalize(char*const* action)
{
#ifdef HAVE_TRACING
  TRACE_smpi_finalize(get_rank(MSG_process_get_name(MSG_process_self())));
#endif
  process_globals_t globals = (process_globals_t) MSG_process_get_data(MSG_process_self());
  if (globals){
    xbt_dynar_free_container(&(globals->isends));
    xbt_dynar_free_container(&(globals->irecvs));
    xbt_dynar_free_container(&(globals->tasks));
    free(globals);
  }
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  factory_initializer_to_use = SIMIX_ctx_raw_factory_init;
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
