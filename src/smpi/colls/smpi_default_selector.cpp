/* selector with default/naive Simgrid algorithms. These should not be trusted for performance evaluations */

/* Copyright (c) 2009-2010, 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"
#include "smpi_process.hpp"

namespace simgrid{
namespace smpi{

int Coll_bcast_default::bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return Coll_bcast_binomial_tree::bcast(buf, count, datatype, root, comm);
}

int Coll_barrier_default::barrier(MPI_Comm comm)
{
  return Coll_barrier_ompi_basic_linear::barrier(comm);
}


int Coll_gather_default::gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
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
    recvtype->extent(&lb, &recvext);
    // Local copy from root
    Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + root * recvcount * recvext,
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
      Request::unref(&requests[src]);
    }
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}

int Coll_reduce_scatter_default::reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op,
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
  void *tmpbuf = static_cast<void*>(smpi_get_tmp_sendbuffer(count*datatype->get_extent()));
  int ret = MPI_SUCCESS;

  ret = Coll_reduce_default::reduce(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Colls::scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  xbt_free(displs);
  smpi_free_tmp_buffer(tmpbuf);
  return ret;
}


int Coll_allgather_default::allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  // FIXME: check for errors
  recvtype->extent(&lb, &recvext);
  // Local copy from self
  Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount,
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
    Request::unref(&requests[other]);
  }
  xbt_free(requests);
  return MPI_SUCCESS;
}

int Coll_allgatherv_default::allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  recvtype->extent(&lb, &recvext);
  // Local copy from self
  Datatype::copy(sendbuf, sendcount, sendtype,
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
    Request::unref(&requests[other]);
  }
  xbt_free(requests);
  return MPI_SUCCESS;
}

int Coll_scatter_default::scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
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
    sendtype->extent(&lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
        Datatype::copy(static_cast<char *>(sendbuf) + root * sendcount * sendext,
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
      Request::unref(&requests[dst]);
    }
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}



int Coll_reduce_default::reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  int system_tag = COLL_TAG_REDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;

  char* sendtmpbuf = static_cast<char *>(sendbuf);

  int rank = comm->rank();
  int size = comm->size();
  if(size==0)
    return MPI_ERR_COMM;
  //non commutative case, use a working algo from openmpi
  if (op != MPI_OP_NULL && not op->is_commutative()) {
    return Coll_reduce_ompi_basic_linear::reduce(sendtmpbuf, recvbuf, count, datatype, op, root, comm);
  }

  if( sendbuf == MPI_IN_PLACE ) {
    sendtmpbuf = static_cast<char *>(smpi_get_tmp_sendbuffer(count*datatype->get_extent()));
    Datatype::copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
  }

  if(rank != root) {
    // Send buffer to root
    Request::send(sendtmpbuf, count, datatype, root, system_tag, comm);
  } else {
    datatype->extent(&lb, &dataext);
    // Local copy from root
    if (sendtmpbuf != nullptr && recvbuf != nullptr)
      Datatype::copy(sendtmpbuf, count, datatype, recvbuf, count, datatype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    void **tmpbufs = xbt_new(void *, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if (src != root) {
        if (not smpi_process()->replaying())
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
        Request::unref(&requests[index]);
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
  return MPI_SUCCESS;
}

int Coll_allreduce_default::allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int ret;
  ret = Colls::reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Colls::bcast(recvbuf, count, datatype, 0, comm);
  return ret;
}

int Coll_alltoall_default::alltoall( void *sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount, MPI_Datatype rdtype, MPI_Comm comm)
{
  return Coll_alltoall_ompi::alltoall(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}



int Coll_alltoallv_default::alltoallv(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = 889;
  int i;
  int count;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  int rank = comm->rank();
  int size = comm->size();
  XBT_DEBUG("<%d> algorithm basic_alltoallv() called.", rank);
  sendtype->extent(&lb, &sendext);
  recvtype->extent(&lb, &recvext);
  /* Local copy from self */
  int err = Datatype::copy(static_cast<char *>(sendbuf) + senddisps[rank] * sendext, sendcounts[rank], sendtype,
                               static_cast<char *>(recvbuf) + recvdisps[rank] * recvext, recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i != rank && recvcounts[i] != 0) {
        requests[count] = Request::irecv_init(static_cast<char *>(recvbuf) + recvdisps[i] * recvext,
                                          recvcounts[i], recvtype, i, system_tag, comm);
        count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcounts[src] = %d]", rank, i, recvcounts[i]);
      }
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i != rank && sendcounts[i] != 0) {
      requests[count] = Request::isend_init(static_cast<char *>(sendbuf) + senddisps[i] * sendext,
                                        sendcounts[i], sendtype, i, system_tag, comm);
      count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]", rank, i, sendcounts[i]);
      }
    }
    /* Wait for them all. */
    Request::startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    Request::waitall(count, requests, MPI_STATUS_IGNORE);
    for(i = 0; i < count; i++) {
      if(requests[i]!=MPI_REQUEST_NULL)
        Request::unref(&requests[i]);
    }
    xbt_free(requests);
  }
  return err;
}

}
}

