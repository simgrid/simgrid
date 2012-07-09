/* Copyright (c) 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <mpi.h>
#include <xbt.h>
#include <xbt/replay.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(smpi_replay,"Trace Replay with SMPI");

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
  smpi_sample_flops(parse_double(action[2]));
}

static void action_send(const char *const *action)
{
  int to = atoi(action[2]);
  const char *size_str = action[3];
  double size=parse_double(size_str);
  char *buffer = (char*) malloc (size);

  XBT_DEBUG("send %.0f bytes to rank%d (%s)",size, to, action[2]);

  MPI_Send(buffer, size, MPI_BYTE, to, 0, MPI_COMM_WORLD);

  free(buffer);
}

static void action_Isend(const char *const *action)
{
  int to = atoi(action[2]);
  const char *size_str = action[3];
  double size=parse_double(size_str);
  char *buffer = (char*) malloc (size);

  MPI_Request request;

  MPI_Isend(buffer, size, MPI_BYTE, to, 0, MPI_COMM_WORLD, &request);

  free(buffer);
}

static void action_recv(const char *const *action)
{
  int from = atoi(action[2]);
  XBT_DEBUG("receive from rank%d (%s)",from, action[2]);
  char *buffer = (char*) malloc (1e10);

  MPI_Status status;

  MPI_Recv(buffer, 1e9, MPI_BYTE, from, 0, MPI_COMM_WORLD, &status);

  free(buffer);
}

int main(int argc, char *argv[])
{
  int i;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &i);
  _xbt_replay_action_init();

//  xbt_replay_action_register("init",     action_init);
//  xbt_replay_action_register("finalize", action_finalize);
//  xbt_replay_action_register("comm_size",action_comm_size);
  xbt_replay_action_register("send",     action_send);
  xbt_replay_action_register("Isend",    action_Isend);
  xbt_replay_action_register("recv",     action_recv);
//  xbt_replay_action_register("Irecv",    action_Irecv);
//  xbt_replay_action_register("wait",     action_wait);
//  xbt_replay_action_register("barrier",  action_barrier);
//  xbt_replay_action_register("bcast",    action_bcast);
//  xbt_replay_action_register("reduce",   action_reduce);
//  xbt_replay_action_register("allReduce",action_allReduce);
//  xbt_replay_action_register("sleep",    action_sleep);
  xbt_replay_action_register("compute",  action_compute);

  xbt_replay_action_runner(argc, argv);
  /* Actually do the simulation using MSG_action_trace_run */
  smpi_action_trace_run(argv[1]);  // it's ok to pass a NULL argument here

  XBT_DEBUG("rank%d: I'm done!",i);
  MPI_Finalize();
  return 0;
}
