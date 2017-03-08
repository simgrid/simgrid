/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.hpp>
#include <algorithm>

#include "private.h"
#include "xbt/virtu.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "xbt/replay.h"
#include <errno.h>
#include "src/simix/smx_private.h"
#include "surf/surf.h"
#include "simgrid/sg_config.h"
#include "smpi/smpi_utils.hpp"
#include "colls/colls.h"
#include <simgrid/s4u/host.hpp>

#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi, "Logging specific to SMPI (base)");


static simgrid::config::Flag<double> smpi_wtime_sleep(
  "smpi/wtime", "Minimum time to inject inside a call to MPI_Wtime", 0.0);
static simgrid::config::Flag<double> smpi_init_sleep(
  "smpi/init", "Time to inject inside a call to MPI_Init", 0.0);

void smpi_mpi_init() {
  if(smpi_init_sleep > 0) 
    simcall_process_sleep(smpi_init_sleep);
}

double smpi_mpi_wtime(){
  double time;
  if (smpi_process_initialized() != 0 && smpi_process_finalized() == 0 && smpi_process_get_sampling() == 0) {
    smpi_bench_end();
    time = SIMIX_get_clock();
    // to avoid deadlocks if used as a break condition, such as
    //     while (MPI_Wtime(...) < time_limit) {
    //       ....
    //     }
    // because the time will not normally advance when only calls to MPI_Wtime
    // are made -> deadlock (MPI_Wtime never reaches the time limit)
    if(smpi_wtime_sleep > 0) 
      simcall_process_sleep(smpi_wtime_sleep);
    smpi_bench_begin();
  } else {
    time = SIMIX_get_clock();
  }
  return time;
}


void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  smpi_coll_tuned_bcast_binomial_tree(buf, count, datatype, root, comm);
}

void smpi_mpi_barrier(MPI_Comm comm)
{
  smpi_coll_tuned_barrier_ompi_basic_linear(comm);
}

void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_GATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Send buffer to root
    Request::send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + root * recvcount * recvext,
                       recvcount, recvtype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = Request::irecv_init(static_cast<char*>(recvbuf) + src * recvcount * recvext, recvcount, recvtype,
                                          src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int src = 0; src < size-1; src++) {
      Request::unuse(&requests[src]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm)
{
  int rank = comm->rank();

  /* arbitrarily choose root as rank 0 */
  int size = comm->size();
  int count = 0;
  int *displs = xbt_new(int, size);
  for (int i = 0; i < size; i++) {
    displs[i] = count;
    count += recvcounts[i];
  }
  void *tmpbuf = static_cast<void*>(smpi_get_tmp_sendbuffer(count*smpi_datatype_get_extent(datatype)));

  mpi_coll_reduce_fun(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  smpi_mpi_scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  xbt_free(displs);
  smpi_free_tmp_buffer(tmpbuf);
}

void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_GATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if (rank != root) {
    // Send buffer to root
    Request::send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + displs[root] * recvext,
                       recvcounts[root], recvtype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = Request::irecv_init(static_cast<char*>(recvbuf) + displs[src] * recvext,
                          recvcounts[src], recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int src = 0; src < size-1; src++) {
      Request::unuse(&requests[src]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  // FIXME: check for errors
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount,
                     recvtype);
  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  int index = 0;
  for (int other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] = Request::isend_init(sendbuf, sendcount, sendtype, other, system_tag,comm);
      index++;
      requests[index] = Request::irecv_init(static_cast<char *>(recvbuf) + other * recvcount * recvext, recvcount, recvtype,
                                        other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  Request::startall(2 * (size - 1), requests);
  Request::waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < 2*(size-1); other++) {
    Request::unuse(&requests[other]);
  }
  xbt_free(requests);
}

void smpi_mpi_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype,
                     static_cast<char *>(recvbuf) + displs[rank] * recvext,recvcounts[rank], recvtype);
  // Send buffers to others;
  MPI_Request *requests = xbt_new(MPI_Request, 2 * (size - 1));
  int index = 0;
  for (int other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
        Request::isend_init(sendbuf, sendcount, sendtype, other, system_tag, comm);
      index++;
      requests[index] = Request::irecv_init(static_cast<char *>(recvbuf) + displs[other] * recvext, recvcounts[other],
                          recvtype, other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  Request::startall(2 * (size - 1), requests);
  Request::waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < 2*(size-1); other++) {
    Request::unuse(&requests[other]);
  }
  xbt_free(requests);
}

void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_SCATTER;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Recv buffer from root
    Request::recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
        smpi_datatype_copy(static_cast<char *>(sendbuf) + root * sendcount * sendext,
                           sendcount, sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for(int dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = Request::isend_init(static_cast<char *>(sendbuf) + dst * sendcount * sendext, sendcount, sendtype,
                                          dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int dst = 0; dst < size-1; dst++) {
      Request::unuse(&requests[dst]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_SCATTERV;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Recv buffer from root
    Request::recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
      smpi_datatype_copy(static_cast<char *>(sendbuf) + displs[root] * sendext, sendcounts[root],
                       sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int dst = 0; dst < size; dst++) {
      if (dst != root) {
        requests[index] = Request::isend_init(static_cast<char *>(sendbuf) + displs[dst] * sendext, sendcounts[dst],
                            sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int dst = 0; dst < size-1; dst++) {
      Request::unuse(&requests[dst]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  int system_tag = COLL_TAG_REDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;

  char* sendtmpbuf = static_cast<char *>(sendbuf);

  int rank = comm->rank();
  int size = comm->size();
  //non commutative case, use a working algo from openmpi
  if(op != MPI_OP_NULL && !op->is_commutative()){
    smpi_coll_tuned_reduce_ompi_basic_linear(sendtmpbuf, recvbuf, count, datatype, op, root, comm);
    return;
  }

  if( sendbuf == MPI_IN_PLACE ) {
    sendtmpbuf = static_cast<char *>(smpi_get_tmp_sendbuffer(count*smpi_datatype_get_extent(datatype)));
    smpi_datatype_copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
  }
  
  if(rank != root) {
    // Send buffer to root
    Request::send(sendtmpbuf, count, datatype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(datatype, &lb, &dataext);
    // Local copy from root
    if (sendtmpbuf != nullptr && recvbuf != nullptr)
      smpi_datatype_copy(sendtmpbuf, count, datatype, recvbuf, count, datatype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    void **tmpbufs = xbt_new(void *, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if (src != root) {
         if (!smpi_process_get_replaying())
          tmpbufs[index] = xbt_malloc(count * dataext);
         else
           tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
        requests[index] =
          Request::irecv_init(tmpbufs[index], count, datatype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    Request::startall(size - 1, requests);
    for (int src = 0; src < size - 1; src++) {
      index = Request::waitany(size - 1, requests, MPI_STATUS_IGNORE);
      XBT_DEBUG("finished waiting any request with index %d", index);
      if(index == MPI_UNDEFINED) {
        break;
      }else{
        Request::unuse(&requests[index]);
      }
      if(op) /* op can be MPI_OP_NULL that does nothing */
        if(op!=MPI_OP_NULL) op->apply( tmpbufs[index], recvbuf, &count, datatype);
    }
      for(index = 0; index < size - 1; index++) {
        smpi_free_tmp_buffer(tmpbufs[index]);
      }
    xbt_free(tmpbufs);
    xbt_free(requests);

  }
  if( sendbuf == MPI_IN_PLACE ) {
    smpi_free_tmp_buffer(sendtmpbuf);
  }
}

void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  smpi_mpi_bcast(recvbuf, count, datatype, 0, comm);
}

void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb      = 0;
  MPI_Aint dataext = 0;

  int rank = comm->rank();
  int size = comm->size();

  smpi_datatype_extent(datatype, &lb, &dataext);

  // Local copy from self
  smpi_datatype_copy(sendbuf, count, datatype, recvbuf, count, datatype);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = Request::irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = Request::isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  Request::startall(size - 1, requests);

  if(op != MPI_OP_NULL && op->is_commutative()){
    for (int other = 0; other < size - 1; other++) {
      index = Request::waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        // #Request is below rank: it's a irecv
        if(op!=MPI_OP_NULL) op->apply( tmpbufs[index], recvbuf, &count, datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
      Request::wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        if(op!=MPI_OP_NULL) op->apply( tmpbufs[other], recvbuf, &count, datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    Request::unuse(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}

void smpi_mpi_exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb         = 0;
  MPI_Aint dataext    = 0;
  int recvbuf_is_empty=1;
  int rank = comm->rank();
  int size = comm->size();

  smpi_datatype_extent(datatype, &lb, &dataext);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = Request::irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = Request::isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  Request::startall(size - 1, requests);

  if(op != MPI_OP_NULL && op->is_commutative()){
    for (int other = 0; other < size - 1; other++) {
      index = Request::waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        if(recvbuf_is_empty){
          smpi_datatype_copy(tmpbufs[index], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty=0;
        } else
          // #Request is below rank: it's a irecv
          if(op!=MPI_OP_NULL) op->apply( tmpbufs[index], recvbuf, &count, datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
     Request::wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        if (recvbuf_is_empty) {
          smpi_datatype_copy(tmpbufs[other], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty = 0;
        } else
          if(op!=MPI_OP_NULL) op->apply( tmpbufs[other], recvbuf, &count, datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    Request::unuse(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}

void smpi_empty_status(MPI_Status * status)
{
  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG = MPI_ANY_TAG;
    status->MPI_ERROR = MPI_SUCCESS;
    status->count=0;
  }
}

int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / smpi_datatype_size(datatype);
}


