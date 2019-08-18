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


int Coll_gather_default::gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  Colls::igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int Coll_reduce_scatter_default::reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm)
{
  int rank = comm->rank();

  /* arbitrarily choose root as rank 0 */
  int size = comm->size();
  int count = 0;
  int* displs = new int[size];
  for (int i = 0; i < size; i++) {
    displs[i] = count;
    count += recvcounts[i];
  }
  unsigned char* tmpbuf = smpi_get_tmp_sendbuffer(count * datatype->get_extent());

  int ret = Coll_reduce_default::reduce(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Colls::scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  delete[] displs;
  smpi_free_tmp_buffer(tmpbuf);
  return ret;
}


int Coll_allgather_default::allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &request);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int Coll_allgatherv_default::allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &request, 0);
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

int Coll_scatter_default::scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  Colls::iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int Coll_reduce_default::reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  //non commutative case, use a working algo from openmpi
  if (op != MPI_OP_NULL && (datatype->flags() & DT_FLAG_DERIVED || not op->is_commutative())) {
    return Coll_reduce_ompi_basic_linear::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  }
  MPI_Request request;
  Colls::ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int Coll_allreduce_default::allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  //FIXME: have mpi_ireduce and iallreduce handle derived datatypes correctly
  if(datatype->flags() & DT_FLAG_DERIVED)
    return Coll_allreduce_ompi::allreduce(sendbuf, recvbuf, count, datatype, op, comm);
  int ret;
  ret = Coll_reduce_default::reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  if(ret==MPI_SUCCESS)
    ret = Coll_bcast_default::bcast(recvbuf, count, datatype, 0, comm);
  return ret;
}

int Coll_alltoall_default::alltoall(const void *sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount, MPI_Datatype rdtype, MPI_Comm comm)
{
  return Coll_alltoall_ompi::alltoall(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}

int Coll_alltoallv_default::alltoallv(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  MPI_Request request;
  Colls::ialltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

}
}

