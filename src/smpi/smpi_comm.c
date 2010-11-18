/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_comm, smpi,
                                "Logging specific to SMPI (comm)");

typedef struct s_smpi_mpi_communicator {
  MPI_Group group;
} s_smpi_mpi_communicator_t;

static int smpi_compare_rankmap(const void *a, const void *b)
{
  const int* x = (const int*)a;
  const int* y = (const int*)b;

  if (x[1] < y[1]) {
    return -1;
  }
  if (x[1] == y[1]) {
    if (x[0] < y[0]) {
      return -1;
    }
    if (x[0] == y[0]) {
      return 0;
    }
    return 1;
  }
  return 1;
}

MPI_Comm smpi_comm_new(MPI_Group group)
{
  MPI_Comm comm;

  comm = xbt_new(s_smpi_mpi_communicator_t, 1);
  comm->group = group;
  smpi_group_use(comm->group);
  return comm;
}

void smpi_comm_destroy(MPI_Comm comm)
{
  smpi_group_destroy(comm->group);
  xbt_free(comm);
}

MPI_Group smpi_comm_group(MPI_Comm comm)
{
  return comm->group;
}

int smpi_comm_size(MPI_Comm comm)
{
  return smpi_group_size(smpi_comm_group(comm));
}

int smpi_comm_rank(MPI_Comm comm)
{
  return smpi_group_rank(smpi_comm_group(comm), smpi_process_index());
}

MPI_Comm smpi_comm_split(MPI_Comm comm, int color, int key)
{
  int system_tag = 666;
  int index, rank, size, i, j, count, reqs;
  int* sendbuf;
  int* recvbuf;
  int* rankmap;
  MPI_Group group, group_root, group_out;
  MPI_Request* requests;

  group_root = group_out = NULL;
  group = smpi_comm_group(comm);
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  /* Gather all colors and keys on rank 0 */
  sendbuf = xbt_new(int, 2);
  sendbuf[0] = color;
  sendbuf[1] = key;
  if(rank == 0) {
    recvbuf = xbt_new(int, 2 * size);
  } else {
    recvbuf = NULL;
  }
  smpi_mpi_gather(sendbuf, 2, MPI_INT, recvbuf, 2, MPI_INT, 0, comm);
  xbt_free(sendbuf);
  /* Do the actual job */
  if(rank == 0) {
    rankmap = xbt_new(int, 2 * size);
    for(i = 0; i < size; i++) {
      if(recvbuf[2 * i] == MPI_UNDEFINED) {
        continue;
      }
      count = 0;
      for(j = i + 1; j < size; j++)  {
        if(recvbuf[2 * i] == recvbuf[2 * j]) {
          recvbuf[2 * j] = MPI_UNDEFINED;
          rankmap[2 * count] = j;
          rankmap[2 * count + 1] = recvbuf[2 * j + 1];
          count++;
        }
      }
      /* Add self in the group */
      recvbuf[2 * i] = MPI_UNDEFINED;
      rankmap[2 * count] = i;
      rankmap[2 * count + 1] = recvbuf[2 * i + 1];
      count++;
      qsort(rankmap, count, 2 * sizeof(int), &smpi_compare_rankmap);
      group_out = smpi_group_new(count);
      if(i == 0) {
        group_root = group_out; /* Save root's group */
      }
      for(j = 0; j < count; j++) {
        index = smpi_group_index(group, rankmap[2 * j]);
        smpi_group_set_mapping(group_out, index, j);
      }
      requests = xbt_new(MPI_Request, count);
      reqs = 0;
      for(j = 0; j < count; j++) {
        if(rankmap[2 * j] != 0) {
          requests[reqs] = smpi_isend_init(&group_out, 1, MPI_PTR, rankmap[2 * j], system_tag, comm);
          reqs++;
        }
      }
      smpi_mpi_startall(reqs, requests);
      smpi_mpi_waitall(reqs, requests, MPI_STATUS_IGNORE);
      xbt_free(requests);
    }
    xbt_free(recvbuf);
    group_out = group_root; /* exit with root's group */
  } else {
    if(color != MPI_UNDEFINED) {
      smpi_mpi_recv(&group_out, 1, MPI_PTR, 0, system_tag, comm, MPI_STATUS_IGNORE);
    } /* otherwise, exit with group_out == NULL */
  }
  return group_out ? smpi_comm_new(group_out) : MPI_COMM_NULL;
}
