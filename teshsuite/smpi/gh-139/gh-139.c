/* Copyright (c) 2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mpi.h>
#include <simgrid/msg.h>
#include <simgrid/simix.h>
#include <stdio.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(smpi_test, "Messages specific for this SMPI example");

struct param {
  MPI_Request* req;
  int rank;
};

struct threadwrap {
  void* father_data;
  void* (*f)(void*);
  void* param;
};

static int global_rank;
void* req_wait(void* bar);

// Thread creation helper

static int thread_create_wrapper(int argc, char* argv[])
{
  int the_global_rank  = global_rank;
  struct threadwrap* t = (struct threadwrap*)sg_actor_self_data();
  XBT_INFO("new thread has parameter rank %d and global variable rank %d", ((struct param*)(t->param))->rank,
           the_global_rank);
  sg_actor_self_data_set(t->father_data);
  t->f(t->param);
  sg_actor_self_data_set(NULL);
  free(t);
  return 0;
}

static void mpi_thread_create(const char* name, void* (*f)(void*), void* param)
{
  struct threadwrap* threadwrap = (struct threadwrap*)malloc(sizeof(*threadwrap));
  threadwrap->father_data       = sg_actor_self_data();
  threadwrap->f                 = f;
  threadwrap->param             = param;
  sg_actor_t actor              = sg_actor_init(name, sg_host_self());
  sg_actor_data_set(actor, threadwrap);
  sg_actor_start(actor, thread_create_wrapper, 0, NULL);
}

static void mythread_create(const char* name, MPI_Request* req, int rank)
{
  struct param* param = (struct param*)malloc(sizeof(*param));
  param->req          = req;
  param->rank         = rank;
  mpi_thread_create(name, req_wait, param);
}

// Actual application

void* req_wait(void* bar)
{
  struct param* param = (struct param*)bar;
  int rank;
  MPI_Status status;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  XBT_INFO("%d has MPI rank %d and global variable rank %d", param->rank, rank, global_rank);
  XBT_INFO("%d waiting request", rank);
  MPI_Wait(param->req, &status);
  XBT_INFO("%d request done", rank);
  XBT_INFO("%d still has MPI rank %d and global variable %d", param->rank, rank, global_rank);
  free(param);
  return NULL;
}

int main(int argc, char* argv[])
{
  int rank, size;
  char c;
  MPI_Request req;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  XBT_INFO("I'm %d/%d", rank, size);
  global_rank = rank;

  if (rank == 0) {
    c = 42;
    MPI_Isend(&c, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &req);
    mythread_create("wait send", &req, rank);
  } else if (rank == 1) {
    MPI_Irecv(&c, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &req);
    mythread_create("wait recv", &req, rank);
  }

  sg_actor_sleep_for(1 + rank);
  MPI_Finalize();
  XBT_INFO("finally %d", c);
  return 0;
}
