/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "colls/colls.h"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi, "Logging specific to SMPI (coll)");

s_mpi_coll_description_t mpi_coll_gather_description[] = {
  {"default", "gather default collective", (void*)smpi_mpi_gather},
   COLL_GATHERS(COLL_DESCRIPTION, COLL_COMMA), {NULL, NULL, NULL}      /* this array must be NULL terminated */
};


s_mpi_coll_description_t mpi_coll_allgather_description[] = {
  {"default",
   "allgather default collective",
   (void*)smpi_mpi_allgather},
COLL_ALLGATHERS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_allgatherv_description[] = {
  {"default",
   "allgatherv default collective",
   (void*)smpi_mpi_allgatherv},
COLL_ALLGATHERVS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_allreduce_description[] = {
  {"default",
   "allreduce default collective",
   (void*)smpi_mpi_allreduce},
COLL_ALLREDUCES(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_reduce_scatter_description[] = {
  {"default",
   "reduce_scatter default collective",
   (void*)smpi_mpi_reduce_scatter},
COLL_REDUCE_SCATTERS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_scatter_description[] = {
  {"default",
   "scatter default collective",
   (void*)smpi_mpi_scatter},
COLL_SCATTERS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_barrier_description[] = {
  {"default",
   "barrier default collective",
   (void*)smpi_mpi_barrier},
COLL_BARRIERS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};
s_mpi_coll_description_t mpi_coll_alltoall_description[] = {
  {"default",
   "Ompi alltoall default collective",
   (void*)smpi_coll_tuned_alltoall_ompi2},
COLL_ALLTOALLS(COLL_DESCRIPTION, COLL_COMMA),
  {"bruck",
   "Alltoall Bruck (SG) collective",
   (void*)smpi_coll_tuned_alltoall_bruck},
  {"basic_linear",
   "Alltoall basic linear (SG) collective",
   (void*)smpi_coll_tuned_alltoall_basic_linear},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_alltoallv_description[] = {
  {"default",
   "Ompi alltoallv default collective",
   (void*)smpi_coll_basic_alltoallv},
COLL_ALLTOALLVS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_bcast_description[] = {
  {"default",
   "bcast default collective ",
   (void*)smpi_mpi_bcast},
COLL_BCASTS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_reduce_description[] = {
  {"default",
   "reduce default collective",
   (void*)smpi_mpi_reduce},
COLL_REDUCES(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};



/** Displays the long description of all registered models, and quit */
void coll_help(const char *category, s_mpi_coll_description_t * table)
{
  int i;
  printf("Long description of the %s models accepted by this simulator:\n",
         category);
  for (i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_coll_description(s_mpi_coll_description_t * table,
                           char *name, const char *desc)
{
  int i;
  char *name_list = NULL;
  int selector_on=0;
  if(name==NULL){//no argument provided, use active selector's algorithm
    name=(char*)sg_cfg_get_string("smpi/coll_selector");
    selector_on=1;
  }
  for (i = 0; table[i].name; i++)
    if (!strcmp(name, table[i].name)) {
      if (strcmp(table[i].name,"default"))
        XBT_INFO("Switch to algorithm %s for collective %s",table[i].name,desc);
      return i;
    }

  if(selector_on){
    // collective seems not handled by the active selector, try with default one
    name=(char*)"default";
    for (i = 0; table[i].name; i++)
      if (!strcmp(name, table[i].name)) {
        return i;
    }
  }
  if (!table[0].name)
    xbt_die("No collective is valid for '%s'! This is a bug.",name);
  name_list = xbt_strdup(table[0].name);
  for (i = 1; table[i].name; i++) {
    name_list = static_cast<char*>(xbt_realloc(name_list,
                    strlen(name_list) + strlen(table[i].name) + 3));
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_die("Collective '%s' is invalid! Valid collectives are: %s.", name, name_list);
  return -1;
}

int (*mpi_coll_gather_fun)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
int (*mpi_coll_allgather_fun)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*mpi_coll_allgatherv_fun)(void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
int (*mpi_coll_allreduce_fun)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int (*mpi_coll_alltoall_fun)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*mpi_coll_alltoallv_fun)(void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
int (*mpi_coll_bcast_fun)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
int (*mpi_coll_reduce_fun)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int (*mpi_coll_reduce_scatter_fun)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int (*mpi_coll_scatter_fun)(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
int (*mpi_coll_barrier_fun)(MPI_Comm comm);


int smpi_coll_tuned_alltoall_ompi2(void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm)
{
  int size, sendsize;  
  size = smpi_comm_size(comm);  
  sendsize = smpi_datatype_size(sendtype) * sendcount;  
  if (sendsize < 200 && size > 12) {
    return
        smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype,
                                       comm);
  } else if (sendsize < 3000) {
    return
        smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount,
                                              sendtype, recvbuf,
                                              recvcount, recvtype, comm);
  } else {
    return
        smpi_coll_tuned_alltoall_ring(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcount, recvtype,
                                      comm);
  }
}

/**
 * Alltoall Bruck
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 * FIXME: uh, check smpi_pmpi again, but this routine is called for > 12, not
 * less...
 **/
int smpi_coll_tuned_alltoall_bruck(void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm)
{
  int system_tag = 777;
  int i, rank, size, err, count;
  MPI_Aint lb;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  // FIXME: check implementation
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_bruck() called.", rank);
  smpi_datatype_extent(sendtype, &lb, &sendext);
  smpi_datatype_extent(recvtype, &lb, &recvext);
  /* Local copy from self */
  err =
      smpi_datatype_copy((char *)sendbuf + rank * sendcount * sendext, 
                         sendcount, sendtype, 
                         (char *)recvbuf + rank * recvcount * recvext,
                         recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcount = %d]",
               rank, i, recvcount);
        continue;
      }
      requests[count] =
          smpi_irecv_init((char *)recvbuf + i * recvcount * recvext, recvcount,
                          recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcount = %d]",
               rank, i, sendcount);
        continue;
      }
      requests[count] =
          smpi_isend_init((char *)sendbuf + i * sendcount * sendext, sendcount,
                          sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    for(i = 0; i < count; i++) {
      if(requests[i]!=MPI_REQUEST_NULL) smpi_mpi_request_free(&requests[i]);
    }
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}

/**
 * Alltoall basic_linear (STARMPI:alltoall-simple)
 **/
int smpi_coll_tuned_alltoall_basic_linear(void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype,
                                          MPI_Comm comm)
{
  int system_tag = 888;
  int i, rank, size, err, count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_basic_linear() called.", rank);
  smpi_datatype_extent(sendtype, &lb, &sendext);
  smpi_datatype_extent(recvtype, &lb, &recvext);
  /* simple optimization */
  err = smpi_datatype_copy((char *)sendbuf + rank * sendcount * sendext, 
                           sendcount, sendtype, 
                           (char *)recvbuf + rank * recvcount * recvext, 
                           recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    /* Post all receives first -- a simple optimization */
    count = 0;
    for (i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
      requests[count] =
          smpi_irecv_init((char *)recvbuf + i * recvcount * recvext, recvcount, 
                          recvtype, i, system_tag, comm);
      count++;
    }
    /* Now post all sends in reverse order
     *   - We would like to minimize the search time through message queue
     *     when messages actually arrive in the order in which they were posted.
     * TODO: check the previous assertion
     */
    for (i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size) {
      requests[count] =
          smpi_isend_init((char *)sendbuf + i * sendcount * sendext, sendcount,
                          sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    for(i = 0; i < count; i++) {
      if(requests[i]!=MPI_REQUEST_NULL) smpi_mpi_request_free(&requests[i]);
    }
    xbt_free(requests);
  }
  return err;
}

int smpi_coll_basic_alltoallv(void *sendbuf, int *sendcounts,
                              int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts,
                              int *recvdisps, MPI_Datatype recvtype,
                              MPI_Comm comm)
{
  int system_tag = 889;
  int i, rank, size, err, count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm basic_alltoallv() called.", rank);
  smpi_datatype_extent(sendtype, &lb, &sendext);
  smpi_datatype_extent(recvtype, &lb, &recvext);
  /* Local copy from self */
  err =
      smpi_datatype_copy((char *)sendbuf + senddisps[rank] * sendext, 
                         sendcounts[rank], sendtype,
                         (char *)recvbuf + recvdisps[rank] * recvext, 
                         recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i == rank || recvcounts[i] == 0) {
        XBT_DEBUG
            ("<%d> skip request creation [src = %d, recvcounts[src] = %d]",
             rank, i, recvcounts[i]);
        continue;
      }
      requests[count] =
          smpi_irecv_init((char *)recvbuf + recvdisps[i] * recvext, 
                          recvcounts[i], recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i == rank || sendcounts[i] == 0) {
        XBT_DEBUG
            ("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]",
             rank, i, sendcounts[i]);
        continue;
      }
      requests[count] =
          smpi_isend_init((char *)sendbuf + senddisps[i] * sendext, 
                          sendcounts[i], sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    for(i = 0; i < count; i++) {
      if(requests[i]!=MPI_REQUEST_NULL) smpi_mpi_request_free(&requests[i]);
    }
    xbt_free(requests);
  }
  return err;
}
