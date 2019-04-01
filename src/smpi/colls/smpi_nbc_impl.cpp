/* Asynchronous parts of the basic collective algorithms, meant to be used both for the naive default implementation, but also for non blocking collectives */

/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

namespace simgrid{
namespace smpi{


int Colls::ibarrier(MPI_Comm comm, MPI_Request* request)
{
  int i;
  int size = comm->size();
  int rank = comm->rank();
  MPI_Request* requests;
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_PERSISTENT);
  if (rank > 0) {
    requests = new MPI_Request[2];
    requests[0] = Request::isend (nullptr, 0, MPI_BYTE, 0,
                             COLL_TAG_BARRIER,
                             comm);
    requests[1] = Request::irecv (nullptr, 0, MPI_BYTE, 0,
                             COLL_TAG_BARRIER,
                             comm);
    (*request)->set_nbc_requests(requests, 2);
  }
  else {
    requests = new MPI_Request[(size-1)*2];
    for (i = 1; i < 2*size-1; i+=2) {
        requests[i-1] = Request::irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE,
                                 COLL_TAG_BARRIER, comm
                                 );
        requests[i] = Request::isend(nullptr, 0, MPI_BYTE, (i+1)/2,
                                 COLL_TAG_BARRIER,
                                 comm
                                 );
    }
    (*request)->set_nbc_requests(requests, 2*(size-1));
  }
  return MPI_SUCCESS;
}

int Colls::ibcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request* request)
{
  int i;
  int size = comm->size();
  int rank = comm->rank();
  MPI_Request* requests;
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_PERSISTENT);
  if (rank != root) {
    requests = new MPI_Request[1];
    requests[0] = Request::irecv (buf, count, datatype, root,
                             COLL_TAG_BCAST,
                             comm);
    (*request)->set_nbc_requests(requests, 1);
  }
  else {
    requests = new MPI_Request[size-1];
    int n = 0;
    for (i = 0; i < size; i++) {
      if(i!=root){
        requests[n] = Request::isend(buf, count, datatype, i,
                                 COLL_TAG_BCAST,
                                 comm
                                 );
        n++;
      }
    }
    (*request)->set_nbc_requests(requests, size-1);
  }
  return MPI_SUCCESS;
}

int Colls::iallgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{

  const int system_tag = COLL_TAG_ALLGATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_PERSISTENT);
  // FIXME: check for errors
  recvtype->extent(&lb, &recvext);
  // Local copy from self
  Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount,
                     recvtype);
  // Send/Recv buffers to/from others;
  requests = new MPI_Request[2 * (size - 1)];
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
  Request::startall(2 * (size - 1), requests);
  (*request)->set_nbc_requests(requests, 2 * (size - 1));
  return MPI_SUCCESS;
}

int Colls::iscatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  const int system_tag = COLL_TAG_SCATTER;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_PERSISTENT);
  if(rank != root) {
    requests = new MPI_Request[1];
    // Recv buffer from root
    requests[0] = Request::irecv(recvbuf, recvcount, recvtype, root, system_tag, comm);
    (*request)->set_nbc_requests(requests, 1);
  } else {
    sendtype->extent(&lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
        Datatype::copy(static_cast<char *>(sendbuf) + root * sendcount * sendext,
                           sendcount, sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    requests = new MPI_Request[size - 1];
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
    (*request)->set_nbc_requests(requests, size - 1);
  }
  return MPI_SUCCESS;
}

int Colls::iallgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  const int system_tag = COLL_TAG_ALLGATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_PERSISTENT);
  recvtype->extent(&lb, &recvext);
  // Local copy from self
  Datatype::copy(sendbuf, sendcount, sendtype,
                     static_cast<char *>(recvbuf) + displs[rank] * recvext,recvcounts[rank], recvtype);
  // Send buffers to others;
  MPI_Request *requests = new MPI_Request[2 * (size - 1)];
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
  (*request)->set_nbc_requests(requests, 2 * (size - 1));
  return MPI_SUCCESS;
}

int Colls::ialltoall( void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request){
int system_tag = COLL_TAG_ALLTOALL;
  int i;
  int count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_ALLTOALL, comm, MPI_REQ_PERSISTENT);
  sendtype->extent(&lb, &sendext);
  recvtype->extent(&lb, &recvext);
  /* simple optimization */
  int err = Datatype::copy(static_cast<char *>(sendbuf) + rank * sendcount * sendext, sendcount, sendtype,
                               static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = new MPI_Request[2 * (size - 1)];
    /* Post all receives first -- a simple optimization */
    count = 0;
    for (i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
      requests[count] = Request::irecv_init(static_cast<char *>(recvbuf) + i * recvcount * recvext, recvcount,
                                        recvtype, i, system_tag, comm);
      count++;
    }
    /* Now post all sends in reverse order
     *   - We would like to minimize the search time through message queue
     *     when messages actually arrive in the order in which they were posted.
     * TODO: check the previous assertion
     */
    for (i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size) {
      requests[count] = Request::isend_init(static_cast<char *>(sendbuf) + i * sendcount * sendext, sendcount,
                                        sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    Request::startall(count, requests);
    (*request)->set_nbc_requests(requests, count);
  }
  return MPI_SUCCESS;
}

int Colls::ialltoallv(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request){
  const int system_tag = COLL_TAG_ALLTOALLV;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_ALLTOALLV, comm, MPI_REQ_PERSISTENT);
  sendtype->extent(&lb, &sendext);
  recvtype->extent(&lb, &recvext);
  /* Local copy from self */
  int err = Datatype::copy(static_cast<char *>(sendbuf) + senddisps[rank] * sendext, sendcounts[rank], sendtype,
                               static_cast<char *>(recvbuf) + recvdisps[rank] * recvext, recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = new MPI_Request[2 * (size - 1)];
    int count = 0;
    /* Create all receives that will be posted first */
    for (int i = 0; i < size; ++i) {
      if (i != rank) {
        requests[count] = Request::irecv_init(static_cast<char *>(recvbuf) + recvdisps[i] * recvext,
                                          recvcounts[i], recvtype, i, system_tag, comm);
        count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcounts[src] = %d]", rank, i, recvcounts[i]);
      }
    }
    /* Now create all sends  */
    for (int i = 0; i < size; ++i) {
      if (i != rank) {
      requests[count] = Request::isend_init(static_cast<char *>(sendbuf) + senddisps[i] * sendext,
                                        sendcounts[i], sendtype, i, system_tag, comm);
      count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]", rank, i, sendcounts[i]);
      }
    }
    /* Wait for them all. */
    Request::startall(count, requests);
    (*request)->set_nbc_requests(requests, count);
  }
  return err;
}

int Colls::ialltoallw(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype* sendtypes,
                              void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request *request){
  const int system_tag = COLL_TAG_ALLTOALLV;
  MPI_Request *requests;

  /* Initialize. */
  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_ALLTOALLV, comm, MPI_REQ_PERSISTENT);
  /* Local copy from self */
  int err = (sendcounts[rank]>0 && recvcounts[rank]) ? Datatype::copy(static_cast<char *>(sendbuf) + senddisps[rank], sendcounts[rank], sendtypes[rank],
                               static_cast<char *>(recvbuf) + recvdisps[rank], recvcounts[rank], recvtypes[rank]): MPI_SUCCESS;
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = new MPI_Request[2 * (size - 1)];
    int count = 0;
    /* Create all receives that will be posted first */
    for (int i = 0; i < size; ++i) {
      if (i != rank) {
        requests[count] = Request::irecv_init(static_cast<char *>(recvbuf) + recvdisps[i],
                                          recvcounts[i], recvtypes[i], i, system_tag, comm);
        count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcounts[src] = %d]", rank, i, recvcounts[i]);
      }
    }
    /* Now create all sends  */
    for (int i = 0; i < size; ++i) {
      if (i != rank) {
      requests[count] = Request::isend_init(static_cast<char *>(sendbuf) + senddisps[i] ,
                                        sendcounts[i], sendtypes[i], i, system_tag, comm);
      count++;
      }else{
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]", rank, i, sendcounts[i]);
      }
    }
    /* Wait for them all. */
    Request::startall(count, requests);
    (*request)->set_nbc_requests(requests, count);
  }
  return err;
}

int Colls::igather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
  const int system_tag = COLL_TAG_GATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_GATHER, comm, MPI_REQ_PERSISTENT);
  if(rank != root) {
    // Send buffer to root
    requests = new MPI_Request[1];
    requests[0]=Request::isend(sendbuf, sendcount, sendtype, root, system_tag, comm);
    (*request)->set_nbc_requests(requests, 1);
  } else {
    recvtype->extent(&lb, &recvext);
    // Local copy from root
    Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + root * recvcount * recvext,
                       recvcount, recvtype);
    // Receive buffers from senders
    requests = new MPI_Request[size - 1];
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
    (*request)->set_nbc_requests(requests, size - 1);
  }
  return MPI_SUCCESS;
}

int Colls::igatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
  int system_tag = COLL_TAG_GATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;
  
  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_GATHERV, comm, MPI_REQ_PERSISTENT);
  if (rank != root) {
    // Send buffer to root
    requests = new MPI_Request[1];
    requests[0]=Request::isend(sendbuf, sendcount, sendtype, root, system_tag, comm);
    (*request)->set_nbc_requests(requests, 1);
  } else {
    recvtype->extent(&lb, &recvext);
    // Local copy from root
    Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + displs[root] * recvext,
                       recvcounts[root], recvtype);
    // Receive buffers from senders
    requests = new MPI_Request[size - 1];
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
    (*request)->set_nbc_requests(requests, size - 1);
  }
  return MPI_SUCCESS;
}
int Colls::iscatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
  int system_tag = COLL_TAG_SCATTERV;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Request* requests;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_SCATTERV, comm, MPI_REQ_PERSISTENT);
  if(rank != root) {
    // Recv buffer from root
    requests = new MPI_Request[1];
    requests[0]=Request::irecv(recvbuf, recvcount, recvtype, root, system_tag, comm);
    (*request)->set_nbc_requests(requests, 1);
  } else {
    sendtype->extent(&lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
      Datatype::copy(static_cast<char *>(sendbuf) + displs[root] * sendext, sendcounts[root],
                       sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    MPI_Request *requests = new MPI_Request[size - 1];
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
    (*request)->set_nbc_requests(requests, size - 1);
  }
  return MPI_SUCCESS;
}

int Colls::ireduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm, MPI_Request* request)
{
  const int system_tag = COLL_TAG_REDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;
  MPI_Request* requests;

  char* sendtmpbuf = static_cast<char *>(sendbuf);

  int rank = comm->rank();
  int size = comm->size();

  if (size <= 0)
    return MPI_ERR_COMM;

  if( sendbuf == MPI_IN_PLACE ) {
    sendtmpbuf = static_cast<char *>(smpi_get_tmp_sendbuffer(count*datatype->get_extent()));
    Datatype::copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
  }

  if(rank == root){
    (*request) =  new Request( recvbuf, count, datatype,
                         rank,rank, COLL_TAG_REDUCE, comm, MPI_REQ_PERSISTENT, op);
  }
  else
    (*request) = new Request( nullptr, count, datatype,
                         rank,rank, COLL_TAG_REDUCE, comm, MPI_REQ_PERSISTENT);

  if(rank != root) {
    // Send buffer to root
    requests = new MPI_Request[1];
    requests[0]=Request::isend(sendtmpbuf, count, datatype, root, system_tag, comm);
    (*request)->set_nbc_requests(requests, 1);
  } else {
    datatype->extent(&lb, &dataext);
    // Local copy from root
    if (sendtmpbuf != nullptr && recvbuf != nullptr)
      Datatype::copy(sendtmpbuf, count, datatype, recvbuf, count, datatype);
    // Receive buffers from senders
    MPI_Request *requests = new MPI_Request[size - 1];
    int index = 0;
    for (int src = 0; src < size; src++) {
      if (src != root) {
        requests[index] =
          Request::irecv_init(smpi_get_tmp_sendbuffer(count * dataext), count, datatype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    Request::startall(size - 1, requests);
    (*request)->set_nbc_requests(requests, size - 1);
  }	
  if( sendbuf == MPI_IN_PLACE ) {
    smpi_free_tmp_buffer(sendtmpbuf);
  }
  return MPI_SUCCESS;
}

int Colls::iallreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, MPI_Comm comm, MPI_Request* request)
{

  const int system_tag = COLL_TAG_ALLREDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  (*request) = new Request( recvbuf, count, datatype,
                         rank,rank, COLL_TAG_ALLREDUCE, comm, MPI_REQ_PERSISTENT, op);
  // FIXME: check for errors
  datatype->extent(&lb, &dataext);
  // Local copy from self
  Datatype::copy(sendbuf, count, datatype, recvbuf, count, datatype);
  // Send/Recv buffers to/from others;
  requests = new MPI_Request[2 * (size - 1)];
  int index = 0;
  for (int other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] = Request::isend_init(sendbuf, count, datatype, other, system_tag,comm);
      index++;
      requests[index] = Request::irecv_init(smpi_get_tmp_sendbuffer(count * dataext), count, datatype,
                                        other, system_tag, comm);
      index++;
    }
  }
  Request::startall(2 * (size - 1), requests);
  (*request)->set_nbc_requests(requests, 2 * (size - 1));
  return MPI_SUCCESS;
}

}
}
