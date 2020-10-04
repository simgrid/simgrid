/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"
#include "smpi_request.hpp"

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
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Scan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype),
                   simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Alltoall(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                       recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
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

void mpi_alltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int* old_sendtypes, void *recvbuf, int *recvcnts,
                      int *rdispls, int* old_recvtypes, int* comm, int* ierr){
  int size = simgrid::smpi::Comm::f2c(*comm)->size();
  auto* sendtypes = new MPI_Datatype[size];
  auto* recvtypes = new MPI_Datatype[size];
  for(int i=0; i< size; i++){
    if(FORT_IN_PLACE(sendbuf)!=MPI_IN_PLACE)
      sendtypes[i] = simgrid::smpi::Datatype::f2c(old_sendtypes[i]);
    recvtypes[i] = simgrid::smpi::Datatype::f2c(old_recvtypes[i]);
  }
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Alltoallw( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls,
                        recvtypes, simgrid::smpi::Comm::f2c(*comm));
  delete[] sendtypes;
  delete[] recvtypes;
}

void mpi_exscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr){
 *ierr = MPI_Exscan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_ibarrier_(int* comm, int* request, int* ierr) {
  MPI_Request req;
  *ierr = MPI_Ibarrier(simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ibcast_(void *buf, int* count, int* datatype, int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *ierr = MPI_Ibcast(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ireduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = static_cast<char *>( FORT_BOTTOM(sendbuf));
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Ireduce(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iallreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Iallreduce(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ireduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, simgrid::smpi::Datatype::f2c(*datatype),
                        simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iscatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Iscatter(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iscatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Iscatterv(sendbuf, sendcounts, displs, simgrid::smpi::Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_igather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Igather(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                     recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_igatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype, int* root, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Igatherv(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                     recvbuf, recvcounts, displs, simgrid::smpi::Datatype::f2c(*recvtype), *root, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iallgather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Iallgather(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                        recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iallgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcounts,int* displs, int* recvtype, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Iallgatherv(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                        recvbuf, recvcounts, displs, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_iscan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Iscan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype),
                   simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ialltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Ialltoall(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype),
                       recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ialltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* request, int* ierr) {
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Ialltoallv(sendbuf, sendcounts, senddisps, simgrid::smpi::Datatype::f2c(*sendtype),
                       recvbuf, recvcounts, recvdisps, simgrid::smpi::Datatype::f2c(*recvtype), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ireduce_scatter_block_ (void *sendbuf, void *recvbuf, int* recvcount, int* datatype, int* op, int* comm,
                                 int* request, int* ierr)
{
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Ireduce_scatter_block(sendbuf, recvbuf, *recvcount, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op),
                                  simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_ialltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int* old_sendtypes, void *recvbuf, int *recvcnts,
                      int *rdispls, int* old_recvtypes, int* comm, int* request, int* ierr){
  MPI_Request req;
  int size = simgrid::smpi::Comm::f2c(*comm)->size();
  auto* sendtypes = new MPI_Datatype[size];
  auto* recvtypes = new MPI_Datatype[size];
  for(int i=0; i< size; i++){
    if(FORT_IN_PLACE(sendbuf)!=MPI_IN_PLACE)
      sendtypes[i] = simgrid::smpi::Datatype::f2c(old_sendtypes[i]);
    recvtypes[i] = simgrid::smpi::Datatype::f2c(old_recvtypes[i]);
  }
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Ialltoallw( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls,
                        recvtypes, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
  delete[] sendtypes;
  delete[] recvtypes;
}

void mpi_iexscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr){
  MPI_Request req;
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Iexscan(sendbuf, recvbuf, *count, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

}
