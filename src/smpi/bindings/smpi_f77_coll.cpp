/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"

extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_barrier_(int* comm, int* ierr) {
  *ierr = MPI_Barrier(simgrid::smpi::Comm::f2c(*comm));
}

void mpi_bcast_(void *buf, int* count, int* datatype, int* root, int* comm, int* ierr) {
  *ierr = MPI_Bcast(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_reduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = static_cast<char *>( FORT_BOTTOM(sendbuf));
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, simgrid::smpi::Datatype::f2c(*datatype),
                        simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr) {
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Scatter(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, int* root, int* comm, int* ierr) {
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Scatterv(sendbuf, sendcounts, displs, simgrid::smpi::Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Gather(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                     recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype, int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Gatherv(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                     recvbuf, recvcounts, displs, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allgather(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                        recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcounts,int* displs, int* recvtype, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allgatherv(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                        recvbuf, recvcounts, displs, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_scan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr) {
  *ierr = MPI_Scan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype),
                   simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoall(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                       recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoallv(sendbuf, sendcounts, senddisps, simgrid::smpi::Datatype::f2c(*sendtype),
                       recvbuf, recvcounts, recvdisps, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_reduce_local_ (void *inbuf, void *inoutbuf, int* count, int* datatype, int* op, int* ierr){

 *ierr = MPI_Reduce_local(inbuf, inoutbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op));
}

void mpi_reduce_scatter_block_ (void *sendbuf, void *recvbuf, int* recvcount, int* datatype, int* op, int* comm,
                                int* ierr)
{
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op),
                                  simgrid::smpi::Comm::f2c(*comm));
}

void mpi_alltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int* sendtypes, void *recvbuf, int *recvcnts,
                      int *rdispls, int* recvtypes, int* comm, int* ierr){
 *ierr = MPI_Alltoallw( sendbuf, sendcnts, sdispls, reinterpret_cast<MPI_Datatype*>(sendtypes), recvbuf, recvcnts, rdispls,
                        reinterpret_cast<MPI_Datatype*>(recvtypes), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_exscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr){
 *ierr = MPI_Exscan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

}
