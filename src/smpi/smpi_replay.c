/* Copyright (c) 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "private.h"
#include <stdio.h>
#include <xbt.h>
#include <xbt/replay.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

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

static void action_compute(const char *const *action)
{
  XBT_DEBUG("Start to compute %.0f flops", parse_double(action[2]));
  smpi_execute_flops(parse_double(action[2]));
}

static void action_send(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);

  XBT_DEBUG("send %.0f bytes to rank%d (%s)",size, to, action[2]);

  smpi_mpi_send(NULL, size, MPI_BYTE, to , 0, MPI_COMM_WORLD);
}

static void action_Isend(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);

  MPI_Request request = smpi_mpi_isend(NULL, size, MPI_BYTE, to, 0,
                                       MPI_COMM_WORLD);

  //TODO do something with request
  request = NULL;
}

static void action_recv(const char *const *action) {
  int from = atoi(action[2]);
  XBT_DEBUG("receive from rank%d (%s)",from, action[2]);

  MPI_Status status;
  //TODO find a way to get the message size
  smpi_mpi_recv(NULL, 1e9, MPI_BYTE, from, 0, MPI_COMM_WORLD, &status);

}

static void action_Irecv(const char *const *action)
{
  int from = atoi(action[2]);
  MPI_Request request;

  XBT_DEBUG("Asynchronous receive from rank%d (%s)",from, action[2]);

  //TODO find a way to get the message size
  request = smpi_mpi_irecv(NULL, 1e9, MPI_BYTE, from, 0, MPI_COMM_WORLD);

  //TODO do something with request
  request = NULL;
}

static void action_wait(const char *const *action){
  MPI_Request request;
  MPI_Status status;

  smpi_mpi_wait(&request, &status);
}

static void action_barrier(const char *const *action){
  smpi_mpi_barrier(MPI_COMM_WORLD);
}

void smpi_replay_init(int *argc, char***argv){
  PMPI_Init(argc, argv);
  _xbt_replay_action_init();

//  xbt_replay_action_register("init",     action_init);
//  xbt_replay_action_register("finalize", action_finalize);
//  xbt_replay_action_register("comm_size",action_comm_size);
  xbt_replay_action_register("send",     action_send);
  xbt_replay_action_register("Isend",    action_Isend);
  xbt_replay_action_register("recv",     action_recv);
  xbt_replay_action_register("Irecv",    action_Irecv);
  xbt_replay_action_register("wait",     action_wait);
  xbt_replay_action_register("barrier",  action_barrier);
//  xbt_replay_action_register("bcast",    action_bcast);
//  xbt_replay_action_register("reduce",   action_reduce);
//  xbt_replay_action_register("allReduce",action_allReduce);
//  xbt_replay_action_register("sleep",    action_sleep);
  xbt_replay_action_register("compute",  action_compute);

  xbt_replay_action_runner(*argc, *argv);
}

int smpi_replay_finalize(){
  return PMPI_Finalize();
}
