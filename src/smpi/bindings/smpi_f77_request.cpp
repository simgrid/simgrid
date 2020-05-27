/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_request.hpp"


extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_send_init_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *request = MPI_FORTRAN_REQUEST_NULL;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Send_init(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *tag, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS && req != nullptr) {
    *request = req->add_f();
  }
}

void mpi_isend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *request = MPI_FORTRAN_REQUEST_NULL;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Isend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *tag, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS && req != nullptr) {
    *request = req->add_f();
  }
}

void mpi_irsend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *request = MPI_FORTRAN_REQUEST_NULL;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Irsend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *tag, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS && req != nullptr) {
    *request = req->add_f();
  }
}

void mpi_send_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr) {
  buf = static_cast<char *>(FORT_BOTTOM(buf));
   *ierr = MPI_Send(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *tag, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_rsend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr) {
  buf = static_cast<char *>(FORT_BOTTOM(buf));
   *ierr = MPI_Rsend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *tag, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_sendrecv_(void* sendbuf, int* sendcount, int* sendtype, int* dst, int* sendtag, void *recvbuf, int* recvcount,
                   int* recvtype, int* src, int* recvtag, int* comm, MPI_Status* status, int* ierr) {
  sendbuf = static_cast<char *>( FORT_BOTTOM(sendbuf));
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
   *ierr = MPI_Sendrecv(sendbuf, *sendcount, simgrid::smpi::Datatype::f2c(*sendtype), *dst, *sendtag, recvbuf, *recvcount,
                        simgrid::smpi::Datatype::f2c(*recvtype), *src, *recvtag, simgrid::smpi::Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}

void mpi_recv_init_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *request = MPI_FORTRAN_REQUEST_NULL;
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Recv_init(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *src, *tag, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_irecv_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  *request = MPI_FORTRAN_REQUEST_NULL;
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Irecv(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *src, *tag, simgrid::smpi::Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS && req != nullptr) {
    *request = req->add_f();
  }
}

void mpi_recv_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, MPI_Status* status, int* ierr) {
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Recv(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *src, *tag, simgrid::smpi::Comm::f2c(*comm), status);
}

void mpi_sendrecv_replace_ (void *buf, int* count, int* datatype, int* dst, int* sendtag, int* src, int* recvtag,
                            int* comm, MPI_Status* status, int* ierr)
{
  *ierr = MPI_Sendrecv_replace(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dst, *sendtag, *src,
  *recvtag, simgrid::smpi::Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}

void mpi_ssend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr) {
  *ierr = MPI_Ssend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_ssend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request tmp;
  *request = MPI_FORTRAN_REQUEST_NULL;
  *ierr = MPI_Ssend_init(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS && tmp != nullptr) {
    *request = tmp->add_f();
  }
}

void mpi_bsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int* ierr) {
 *ierr = MPI_Bsend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_bsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *request = MPI_FORTRAN_REQUEST_NULL;
  *ierr = MPI_Bsend_init(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS && tmp != nullptr) {
    *request = tmp->add_f();
  }
}

void mpi_ibsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *request = MPI_FORTRAN_REQUEST_NULL;
  *ierr = MPI_Ibsend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS && tmp != nullptr) {
    *request = tmp->add_f();
  }
}

void mpi_issend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *request = MPI_FORTRAN_REQUEST_NULL;
  *ierr = MPI_Issend(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS && tmp != nullptr) {
    *request = tmp->add_f();
  }
}

void mpi_rsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *request = MPI_FORTRAN_REQUEST_NULL;
  *ierr = MPI_Rsend_init(buf, *count, simgrid::smpi::Datatype::f2c(*datatype), *dest, *tag, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS && tmp != nullptr) {
    *request = tmp->add_f();
  }
}

void mpi_start_(int* request, int* ierr) {
  MPI_Request req = simgrid::smpi::Request::f2c(*request);

  *ierr = MPI_Start(&req);
}

void mpi_startall_(int* count, int* requests, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr = MPI_Startall(*count, reqs);
  xbt_free(reqs);
}

void mpi_wait_(int* request, MPI_Status* status, int* ierr) {
   MPI_Request req = simgrid::smpi::Request::f2c(*request);

   *ierr = MPI_Wait(&req, FORT_STATUS_IGNORE(status));
   if(req==MPI_REQUEST_NULL){
     simgrid::smpi::Request::free_f(*request);
     *request=MPI_FORTRAN_REQUEST_NULL;
   }
}

void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitany(*count, reqs, index, status);
  if(*index!=MPI_UNDEFINED){
    if(reqs[*index]==MPI_REQUEST_NULL){
        simgrid::smpi::Request::free_f(requests[*index]);
        requests[*index]=MPI_FORTRAN_REQUEST_NULL;
    }
  *index=*index+1;
  }
  xbt_free(reqs);
}

void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitall(*count, reqs, FORT_STATUSES_IGNORE(status));
  for(i = 0; i < *count; i++) {
      if(reqs[i]==MPI_REQUEST_NULL){
          simgrid::smpi::Request::free_f(requests[i]);
          requests[i]=MPI_FORTRAN_REQUEST_NULL;
      }
  }

  xbt_free(reqs);
}

void mpi_waitsome_ (int* incount, int* requests, int *outcount, int *indices, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *incount);
  for(i = 0; i < *incount; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitsome(*incount, reqs, outcount, indices, status);
  for(i=0;i<*outcount;i++){
    if(reqs[indices[i]]==MPI_REQUEST_NULL){
        simgrid::smpi::Request::free_f(requests[indices[i]]);
        requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
    }
    indices[i]++;
  }
  xbt_free(reqs);
}

void mpi_test_ (int * request, int *flag, MPI_Status * status, int* ierr){
  MPI_Request req = simgrid::smpi::Request::f2c(*request);
  *ierr= MPI_Test(&req, flag, FORT_STATUS_IGNORE(status));
  if(req==MPI_REQUEST_NULL){
      simgrid::smpi::Request::free_f(*request);
      *request=MPI_FORTRAN_REQUEST_NULL;
  }
}

void mpi_testall_ (int* count, int * requests,  int *flag, MPI_Status * statuses, int* ierr){
  int i;
  MPI_Request* reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr= MPI_Testall(*count, reqs, flag, FORT_STATUSES_IGNORE(statuses));
  for(i = 0; i < *count; i++) {
    if(reqs[i]==MPI_REQUEST_NULL){
        simgrid::smpi::Request::free_f(requests[i]);
        requests[i]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
  xbt_free(reqs);
}

void mpi_testany_ (int* count, int* requests, int *index, int *flag, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
  }
  *ierr = MPI_Testany(*count, reqs, index, flag, FORT_STATUS_IGNORE(status));
  if(*index!=MPI_UNDEFINED){
    if(reqs[*index]==MPI_REQUEST_NULL){
    simgrid::smpi::Request::free_f(requests[*index]);
    requests[*index]=MPI_FORTRAN_REQUEST_NULL;
    }
  *index=*index+1;
  }
  xbt_free(reqs);
}

void mpi_testsome_ (int* incount, int*  requests, int* outcount, int* indices, MPI_Status*  statuses, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *incount);
  for(i = 0; i < *incount; i++) {
    reqs[i] = simgrid::smpi::Request::f2c(requests[i]);
    indices[i]=0;
  }
  *ierr = MPI_Testsome(*incount, reqs, outcount, indices, FORT_STATUSES_IGNORE(statuses));
  for(i=0;i<*incount;i++){
    if(reqs[indices[i]]==MPI_REQUEST_NULL){
      simgrid::smpi::Request::free_f(requests[indices[i]]);
      requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
    }
    indices[i]++;
  }
  xbt_free(reqs);
}

void mpi_probe_ (int* source, int* tag, int* comm, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Probe(*source, *tag, simgrid::smpi::Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}


void mpi_iprobe_ (int* source, int* tag, int* comm, int* flag, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Iprobe(*source, *tag, simgrid::smpi::Comm::f2c(*comm), flag, status);
}

}
