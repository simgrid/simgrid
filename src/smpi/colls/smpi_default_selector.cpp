/* selector with default/naive Simgrid algorithms. These should not be trusted for performance evaluations */

/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

namespace simgrid{
namespace smpi{

int bcast__default(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast__binomial_tree(buf, count, datatype, root, comm);
}

int barrier__default(MPI_Comm comm)
{
  return barrier__ompi_basic_linear(comm);
}


int gather__default(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  colls::igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int reduce_scatter__default(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                            MPI_Comm comm)
{
  int rank = comm->rank();

  /* arbitrarily choose root as rank 0 */
  int size = comm->size();
  int count = 0;
  int* displs = new int[size];
  int regular=1;
  for (int i = 0; i < size; i++) {
    if(recvcounts[i]!=recvcounts[0]){
      regular=0;
      break;
    }
    displs[i] = count;
    count += recvcounts[i];
  }
  if(not regular){
    delete[] displs;
    return reduce_scatter__mpich(sendbuf, recvbuf, recvcounts, datatype, op, comm);
  }

  unsigned char* tmpbuf = smpi_get_tmp_sendbuffer(count * datatype->get_extent());

  int ret = reduce__default(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = colls::scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  delete[] displs;
  smpi_free_tmp_buffer(tmpbuf);
  return ret;
}


int allgather__default(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  colls::iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &request);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int allgatherv__default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  colls::iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &request, 0);
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

int scatter__default(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  colls::iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int reduce__default(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                    MPI_Comm comm)
{
  //non commutative case, use a working algo from openmpi
  if (op != MPI_OP_NULL && (datatype->flags() & DT_FLAG_DERIVED || not op->is_commutative())) {
    return reduce__ompi_basic_linear(sendbuf, recvbuf, count, datatype, op, root, comm);
  }
  MPI_Request request;
  colls::ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int allreduce__default(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  //FIXME: have mpi_ireduce and iallreduce handle derived datatypes correctly
  if(datatype->flags() & DT_FLAG_DERIVED)
    return allreduce__ompi(sendbuf, recvbuf, count, datatype, op, comm);
  int ret;
  ret = reduce__default(sendbuf, recvbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = bcast__default(recvbuf, count, datatype, 0, comm);
  return ret;
}

int alltoall__default(const void *sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount, MPI_Datatype rdtype, MPI_Comm comm)
{
  return alltoall__ompi(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}

int alltoallv__default(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype,
                       void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  colls::ialltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, &request,
                    0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

}
}

