/* Copyright (c) 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "private.h"
#include <stdio.h>
#include <xbt.h>
#include <xbt/replay.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

int communicator_size = 0;

typedef struct {
  xbt_dynar_t isends; /* of MPI_Request */
  xbt_dynar_t irecvs; /* of MPI_Request */
} s_smpi_replay_globals_t, *smpi_replay_globals_t;

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

static void action_init(const char *const *action)
{
  XBT_DEBUG("Initialize the counters");
  smpi_replay_globals_t globals =  xbt_new(s_smpi_replay_globals_t, 1);
  globals->isends = xbt_dynar_new(sizeof(MPI_Request),NULL);
  globals->irecvs = xbt_dynar_new(sizeof(MPI_Request),NULL);

  smpi_process_set_user_data((void*) globals);

  /* be sure that everyone has initiated the counters before proceeding */
  smpi_mpi_barrier(MPI_COMM_WORLD);
  /* start a simulated timer */
  smpi_process_simulated_start();
}

static void action_finalize(const char *const *action)
{
  double sim_time= 1.;
  smpi_replay_globals_t globals =
      (smpi_replay_globals_t) smpi_process_get_user_data();

  if (globals){
    XBT_DEBUG("There are %lu isends and %lu irecvs in the dynars",
         xbt_dynar_length(globals->isends),xbt_dynar_length(globals->irecvs));
    xbt_dynar_free_container(&(globals->isends));
    xbt_dynar_free_container(&(globals->irecvs));
  }
  free(globals);
  /* end the simulated timer */
  sim_time = smpi_process_simulated_elapsed();
  if (!smpi_process_index())
    XBT_INFO("Simulation time %g", sim_time);

}

static void action_comm_size(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();

  communicator_size = parse_double(action[2]);

  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}


static void action_compute(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();
  smpi_execute_flops(parse_double(action[2]));

  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_send(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();

  smpi_mpi_send(NULL, size, MPI_BYTE, to , 0, MPI_COMM_WORLD);
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_Isend(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  smpi_replay_globals_t globals =
     (smpi_replay_globals_t) smpi_process_get_user_data();

  MPI_Request request = smpi_mpi_isend(NULL, size, MPI_BYTE, to, 0,
                                       MPI_COMM_WORLD);

  xbt_dynar_push(globals->isends,&request);

  //TODO do the asynchronous cleanup
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_recv(const char *const *action) {
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Status status;

  smpi_mpi_recv(NULL, size, MPI_BYTE, from, 0, MPI_COMM_WORLD, &status);
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_Irecv(const char *const *action)
{
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;
  smpi_replay_globals_t globals =
     (smpi_replay_globals_t) smpi_process_get_user_data();

  XBT_DEBUG("Asynchronous receive of %.0f from rank%d (%s)",size, from,
            action[2]);

  request = smpi_mpi_irecv(NULL, size, MPI_BYTE, from, 0, MPI_COMM_WORLD);
  xbt_dynar_push(globals->irecvs,&request);

  //TODO do the asynchronous cleanup
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_wait(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;
  MPI_Status status;
  smpi_replay_globals_t globals =
      (smpi_replay_globals_t) smpi_process_get_user_data();

  xbt_assert(xbt_dynar_length(globals->irecvs),
      "action wait not preceded by any irecv: %s",
      xbt_str_join_array(action," "));
  request = xbt_dynar_pop_as(globals->irecvs,MPI_Request);
  smpi_mpi_wait(&request, &status);
  XBT_DEBUG("MPI_Wait Status is : (source=%d tag=%d error=%d count=%d)",
            status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR, status.count);
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_barrier(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
  smpi_mpi_barrier(MPI_COMM_WORLD);
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_bcast(const char *const *action)
{
  double size = parse_double(action[2]);
  double clock = smpi_process_simulated_elapsed();

  smpi_mpi_bcast(NULL, size, MPI_BYTE, 0, MPI_COMM_WORLD);

  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_reduce(const char *const *action)
{
  double size = parse_double(action[2]);
  double clock = smpi_process_simulated_elapsed();
  smpi_mpi_reduce(NULL, NULL, size, MPI_BYTE, MPI_OP_NULL, 0, MPI_COMM_WORLD);

  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

static void action_allReduce(const char *const *action) {
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  smpi_mpi_reduce(NULL, NULL, comm_size, MPI_BYTE, MPI_OP_NULL, 0, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);
  smpi_mpi_bcast(NULL, comm_size, MPI_BYTE, 0, MPI_COMM_WORLD);
  XBT_VERB("%s %f", xbt_str_join_array(action, " "),
           smpi_process_simulated_elapsed()-clock);
}

void smpi_replay_init(int *argc, char***argv){
  PMPI_Init(argc, argv);
  _xbt_replay_action_init();

  xbt_replay_action_register("init",     action_init);
  xbt_replay_action_register("finalize", action_finalize);
  xbt_replay_action_register("comm_size",action_comm_size);
  xbt_replay_action_register("send",     action_send);
  xbt_replay_action_register("Isend",    action_Isend);
  xbt_replay_action_register("recv",     action_recv);
  xbt_replay_action_register("Irecv",    action_Irecv);
  xbt_replay_action_register("wait",     action_wait);
  xbt_replay_action_register("barrier",  action_barrier);
  xbt_replay_action_register("bcast",    action_bcast);
  xbt_replay_action_register("reduce",   action_reduce);
  xbt_replay_action_register("allReduce",action_allReduce);
  xbt_replay_action_register("compute",  action_compute);

  xbt_replay_action_runner(*argc, *argv);
}

int smpi_replay_finalize(){
  return PMPI_Finalize();
}
