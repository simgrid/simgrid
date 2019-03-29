/* selector with default/naive Simgrid algorithms. These should not be trusted for performance evaluations */

/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

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
  MPI_Request request;
  Colls::igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request);
  MPI_Request* requests = request->get_nbc_requests();
  int count = request->get_nbc_requests_size();
  Request::waitall(count, requests, MPI_STATUS_IGNORE);
  for (int i = 0; i < count; i++) {
    if(requests[i]!=MPI_REQUEST_NULL)
      Request::unref(&requests[i]);
  }
  delete[] requests;
  Request::unref(&request);
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

  int ret = Coll_reduce_default::reduce(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Colls::scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  xbt_free(displs);
  smpi_free_tmp_buffer(tmpbuf);
  return ret;
}


int Coll_allgather_default::allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &request);
  MPI_Request* requests = request->get_nbc_requests();
  int count = request->get_nbc_requests_size();
  Request::waitall(count, requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < count; other++) {
    Request::unref(&requests[other]);
  }
  delete[] requests;
  Request::unref(&request);
  return MPI_SUCCESS;
}

int Coll_allgatherv_default::allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &request);
  MPI_Request* requests = request->get_nbc_requests();
  int count = request->get_nbc_requests_size();
  Request::waitall(count, requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < count; other++) {
    Request::unref(&requests[other]);
  }
  delete[] requests;
  Request::unref(&request);
  return MPI_SUCCESS;
}

int Coll_scatter_default::scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request);
  MPI_Request* requests = request->get_nbc_requests();
  int count = request->get_nbc_requests_size();
  Request::waitall(count, requests, MPI_STATUS_IGNORE);
  for (int dst = 0; dst < count; dst++) {
    if(requests[dst]!=MPI_REQUEST_NULL)
      Request::unref(&requests[dst]);
  }
  delete[] requests;
  Request::unref(&request);
  return MPI_SUCCESS;
}



int Coll_reduce_default::reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  const int system_tag = COLL_TAG_REDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;

  char* sendtmpbuf = static_cast<char *>(sendbuf);

  int rank = comm->rank();
  int size = comm->size();
  if (size <= 0)
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
  ret = Coll_reduce_default::reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Coll_bcast_default::bcast(recvbuf, count, datatype, 0, comm);
  return ret;
}

int Coll_alltoall_default::alltoall( void *sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount, MPI_Datatype rdtype, MPI_Comm comm)
{
  return Coll_alltoall_ompi::alltoall(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}

int Coll_alltoallv_default::alltoallv(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  int err = Colls::ialltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, &request);
  MPI_Request* requests = request->get_nbc_requests();
  int count = request->get_nbc_requests_size();
  XBT_DEBUG("<%d> wait for %d requests", comm->rank(), count);
  Request::waitall(count, requests, MPI_STATUS_IGNORE);
  for (int i = 0; i < count; i++) {
    if(requests[i]!=MPI_REQUEST_NULL)
      Request::unref(&requests[i]);
  }
  delete[] requests;
  Request::unref(&request);
  return err;
}

}
}

