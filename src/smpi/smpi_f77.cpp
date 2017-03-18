/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <limits.h>
#include <stdio.h>
#include "private.h"
#include "xbt.h"

static int running_processes = 0;

#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
typedef int integer;
#else
typedef long int integer;
#endif

/* Convert between Fortran and C */

#define FORT_BOTTOM(addr)          ((*(int*)addr) == -200 ? MPI_BOTTOM : (void*)addr)
#define FORT_IN_PLACE(addr)        ((*(int*)addr) == -100 ? MPI_IN_PLACE : (void*)addr)
#define FORT_STATUS_IGNORE(addr)   (static_cast<MPI_Status*>((*(int*)addr) == -300 ? MPI_STATUS_IGNORE : (void*)addr))
#define FORT_STATUSES_IGNORE(addr) (static_cast<MPI_Status*>((*(int*)addr) == -400 ? MPI_STATUSES_IGNORE : (void*)addr))

#define KEY_SIZE (sizeof(int) * 2 + 1)

static char* get_key(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x",id);
  return key;
}

static char* get_key_id(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x_%d",id, smpi_process()->index());
  return key;
}

static void smpi_init_fortran_types(){
   if(F2C::lookup() == nullptr){
     MPI_COMM_WORLD->add_f();
     MPI_BYTE->add_f();//MPI_BYTE
     MPI_CHAR->add_f();//MPI_CHARACTER
#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
     MPI_C_BOOL->add_f();//MPI_LOGICAL
     MPI_INT->add_f();//MPI_INTEGER
#else
     MPI_C_BOOL->add_f();//MPI_LOGICAL
     MPI_LONG->add_f();//MPI_INTEGER
#endif
     MPI_INT8_T->add_f();//MPI_INTEGER1
     MPI_INT16_T->add_f();//MPI_INTEGER2
     MPI_INT32_T->add_f();//MPI_INTEGER4
     MPI_INT64_T->add_f();//MPI_INTEGER8
     MPI_REAL->add_f();//MPI_REAL
     MPI_REAL4->add_f();//MPI_REAL4
     MPI_REAL8->add_f();//MPI_REAL8
     MPI_DOUBLE->add_f();//MPI_DOUBLE_PRECISION
     MPI_C_FLOAT_COMPLEX->add_f();//MPI_COMPLEX
     MPI_C_DOUBLE_COMPLEX->add_f();//MPI_DOUBLE_COMPLEX
#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
     MPI_2INT->add_f();//MPI_2INTEGER
#else
     MPI_2LONG->add_f();//MPI_2INTEGER
#endif
     MPI_UINT8_T->add_f();//MPI_LOGICAL1
     MPI_UINT16_T->add_f();//MPI_LOGICAL2
     MPI_UINT32_T->add_f();//MPI_LOGICAL4
     MPI_UINT64_T->add_f();//MPI_LOGICAL8
     MPI_2FLOAT->add_f();//MPI_2REAL
     MPI_2DOUBLE->add_f();//MPI_2DOUBLE_PRECISION
     MPI_PTR->add_f();//MPI_AINT
     MPI_OFFSET->add_f();//MPI_OFFSET
     MPI_AINT->add_f();//MPI_COUNT
     MPI_REAL16->add_f();//MPI_REAL16
     MPI_PACKED->add_f();//MPI_PACKED

     MPI_MAX->add_f();
     MPI_MIN->add_f();
     MPI_MAXLOC->add_f();
     MPI_MINLOC->add_f();
     MPI_SUM->add_f();
     MPI_PROD->add_f();
     MPI_LAND->add_f();
     MPI_LOR->add_f();
     MPI_LXOR->add_f();
     MPI_BAND->add_f();
     MPI_BOR->add_f();
     MPI_BXOR->add_f();
   }
}

extern "C" { // This should really use the C linkage to be usable from Fortran


void mpi_init_(int* ierr) {
    smpi_init_fortran_types();
   *ierr = MPI_Init(nullptr, nullptr);
   running_processes++;
}

void mpi_finalize_(int* ierr) {
   *ierr = MPI_Finalize();
   running_processes--;
   if(running_processes==0){
      F2C::delete_lookup();
   }
}

void mpi_abort_(int* comm, int* errorcode, int* ierr) {
  *ierr = MPI_Abort(Comm::f2c(*comm), *errorcode);
}

void mpi_comm_rank_(int* comm, int* rank, int* ierr) {
   *ierr = MPI_Comm_rank(Comm::f2c(*comm), rank);
}

void mpi_comm_size_(int* comm, int* size, int* ierr) {
   *ierr = MPI_Comm_size(Comm::f2c(*comm), size);
}

double mpi_wtime_() {
   return MPI_Wtime();
}

double mpi_wtick_() {
  return MPI_Wtick();
}

void mpi_comm_dup_(int* comm, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_dup(Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_comm_create_(int* comm, int* group, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_create(Comm::f2c(*comm),Group::f2c(*group), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_comm_free_(int* comm, int* ierr) {
  MPI_Comm tmp = Comm::f2c(*comm);

  *ierr = MPI_Comm_free(&tmp);

  if(*ierr == MPI_SUCCESS) {
    Comm::free_f(*comm);
  }
}

void mpi_comm_split_(int* comm, int* color, int* key, int* comm_out, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_split(Comm::f2c(*comm), *color, *key, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = tmp->add_f();
  }
}

void mpi_group_incl_(int* group, int* n, int* ranks, int* group_out, int* ierr) {
  MPI_Group tmp;

  *ierr = MPI_Group_incl(Group::f2c(*group), *n, ranks, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = tmp->add_f();
  }
}

void mpi_comm_group_(int* comm, int* group_out,  int* ierr) {
  MPI_Group tmp;

  *ierr = MPI_Comm_group(Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = tmp->c2f();
  }
}

void mpi_initialized_(int* flag, int* ierr){
  *ierr = MPI_Initialized(flag);
}

void mpi_send_init_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Send_init(buf, *count, Datatype::f2c(*datatype), *dst, *tag, Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_isend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Isend(buf, *count, Datatype::f2c(*datatype), *dst, *tag, Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_irsend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = static_cast<char *>(FORT_BOTTOM(buf));
  *ierr = MPI_Irsend(buf, *count, Datatype::f2c(*datatype), *dst, *tag, Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_send_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr) {
  buf = static_cast<char *>(FORT_BOTTOM(buf));
   *ierr = MPI_Send(buf, *count, Datatype::f2c(*datatype), *dst, *tag, Comm::f2c(*comm));
}

void mpi_rsend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr) {
  buf = static_cast<char *>(FORT_BOTTOM(buf));
   *ierr = MPI_Rsend(buf, *count, Datatype::f2c(*datatype), *dst, *tag, Comm::f2c(*comm));
}

void mpi_sendrecv_(void* sendbuf, int* sendcount, int* sendtype, int* dst, int* sendtag, void *recvbuf, int* recvcount,
                   int* recvtype, int* src, int* recvtag, int* comm, MPI_Status* status, int* ierr) {
  sendbuf = static_cast<char *>( FORT_BOTTOM(sendbuf));
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
   *ierr = MPI_Sendrecv(sendbuf, *sendcount, Datatype::f2c(*sendtype), *dst, *sendtag, recvbuf, *recvcount,
                        Datatype::f2c(*recvtype), *src, *recvtag, Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}

void mpi_recv_init_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Recv_init(buf, *count, Datatype::f2c(*datatype), *src, *tag, Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_irecv_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Irecv(buf, *count, Datatype::f2c(*datatype), *src, *tag, Comm::f2c(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_recv_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, MPI_Status* status, int* ierr) {
  buf = static_cast<char *>( FORT_BOTTOM(buf));
  *ierr = MPI_Recv(buf, *count, Datatype::f2c(*datatype), *src, *tag, Comm::f2c(*comm), status);
}

void mpi_start_(int* request, int* ierr) {
  MPI_Request req = Request::f2c(*request);

  *ierr = MPI_Start(&req);
}

void mpi_startall_(int* count, int* requests, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr = MPI_Startall(*count, reqs);
  xbt_free(reqs);
}

void mpi_wait_(int* request, MPI_Status* status, int* ierr) {
   MPI_Request req = Request::f2c(*request);
   
   *ierr = MPI_Wait(&req, FORT_STATUS_IGNORE(status));
   if(req==MPI_REQUEST_NULL){
     Request::free_f(*request);
     *request=MPI_FORTRAN_REQUEST_NULL;
   }
}

void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitany(*count, reqs, index, status);
  if(reqs[*index]==MPI_REQUEST_NULL){
      Request::free_f(requests[*index]);
      requests[*index]=MPI_FORTRAN_REQUEST_NULL;
  }
  xbt_free(reqs);
}

void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitall(*count, reqs, FORT_STATUSES_IGNORE(status));
  for(i = 0; i < *count; i++) {
      if(reqs[i]==MPI_REQUEST_NULL){
          Request::free_f(requests[i]);
          requests[i]=MPI_FORTRAN_REQUEST_NULL;
      }
  }

  xbt_free(reqs);
}

void mpi_barrier_(int* comm, int* ierr) {
  *ierr = MPI_Barrier(Comm::f2c(*comm));
}

void mpi_bcast_(void *buf, int* count, int* datatype, int* root, int* comm, int* ierr) {
  *ierr = MPI_Bcast(buf, *count, Datatype::f2c(*datatype), *root, Comm::f2c(*comm));
}

void mpi_reduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = static_cast<char *>( FORT_BOTTOM(sendbuf));
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count, Datatype::f2c(*datatype), Op::f2c(*op), *root, Comm::f2c(*comm));
}

void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, Datatype::f2c(*datatype), Op::f2c(*op), Comm::f2c(*comm));
}

void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, Datatype::f2c(*datatype),
                        Op::f2c(*op), Comm::f2c(*comm));
}

void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr) {
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Scatter(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, Datatype::f2c(*recvtype), *root, Comm::f2c(*comm));
}

void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, int* root, int* comm, int* ierr) {
  recvbuf = static_cast<char *>( FORT_IN_PLACE(recvbuf));
  *ierr = MPI_Scatterv(sendbuf, sendcounts, displs, Datatype::f2c(*sendtype),
                      recvbuf, *recvcount, Datatype::f2c(*recvtype), *root, Comm::f2c(*comm));
}

void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Gather(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                     recvbuf, *recvcount, Datatype::f2c(*recvtype), *root, Comm::f2c(*comm));
}

void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype, int* root, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  sendbuf = sendbuf!=MPI_IN_PLACE ? static_cast<char *>( FORT_BOTTOM(sendbuf)) : MPI_IN_PLACE;
  recvbuf = static_cast<char *>( FORT_BOTTOM(recvbuf));
  *ierr = MPI_Gatherv(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                     recvbuf, recvcounts, displs, Datatype::f2c(*recvtype), *root, Comm::f2c(*comm));
}

void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allgather(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                        recvbuf, *recvcount, Datatype::f2c(*recvtype), Comm::f2c(*comm));
}

void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcounts,int* displs, int* recvtype, int* comm, int* ierr) {
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
  *ierr = MPI_Allgatherv(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                        recvbuf, recvcounts, displs, Datatype::f2c(*recvtype), Comm::f2c(*comm));
}

void mpi_scan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr) {
  *ierr = MPI_Scan(sendbuf, recvbuf, *count, Datatype::f2c(*datatype),
                   Op::f2c(*op), Comm::f2c(*comm));
}

void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoall(sendbuf, *sendcount, Datatype::f2c(*sendtype),
                       recvbuf, *recvcount, Datatype::f2c(*recvtype), Comm::f2c(*comm));
}

void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoallv(sendbuf, sendcounts, senddisps, Datatype::f2c(*sendtype),
                       recvbuf, recvcounts, recvdisps, Datatype::f2c(*recvtype), Comm::f2c(*comm));
}

void mpi_test_ (int * request, int *flag, MPI_Status * status, int* ierr){
  MPI_Request req = Request::f2c(*request);
  *ierr= MPI_Test(&req, flag, FORT_STATUS_IGNORE(status));
  if(req==MPI_REQUEST_NULL){
      Request::free_f(*request);
      *request=MPI_FORTRAN_REQUEST_NULL;
  }
}

void mpi_testall_ (int* count, int * requests,  int *flag, MPI_Status * statuses, int* ierr){
  int i;
  MPI_Request* reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr= MPI_Testall(*count, reqs, flag, FORT_STATUSES_IGNORE(statuses));
  for(i = 0; i < *count; i++) {
    if(reqs[i]==MPI_REQUEST_NULL){
        Request::free_f(requests[i]);
        requests[i]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
  xbt_free(reqs);
}

void mpi_get_processor_name_(char *name, int *resultlen, int* ierr){
  *ierr = MPI_Get_processor_name(name, resultlen);
}

void mpi_get_count_(MPI_Status * status, int* datatype, int *count, int* ierr){
  *ierr = MPI_Get_count(FORT_STATUS_IGNORE(status), Datatype::f2c(*datatype), count);
}

void mpi_attr_get_(int* comm, int* keyval, void* attr_value, int* flag, int* ierr ){
  *ierr = MPI_Attr_get(Comm::f2c(*comm), *keyval, attr_value, flag);
}

void mpi_type_extent_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(Datatype::f2c(*datatype),  extent);
}

void mpi_type_commit_(int* datatype,  int* ierr){
  MPI_Datatype tmp= Datatype::f2c(*datatype);
  *ierr= MPI_Type_commit(&tmp);
}

void mpi_type_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_vector(*count, *blocklen, *stride, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_vector(*count, *blocklen, *stride, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector (*count, *blocklen, *stride, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector(*count, *blocklen, *stride, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_free_(int* datatype, int* ierr){
  MPI_Datatype tmp= Datatype::f2c(*datatype);
  *ierr= MPI_Type_free (&tmp);
  if(*ierr == MPI_SUCCESS) {
    F2C::free_f(*datatype);
  }
}

void mpi_type_ub_(int* datatype, MPI_Aint * disp, int* ierr){
  *ierr= MPI_Type_ub(Datatype::f2c(*datatype), disp);
}

void mpi_type_lb_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(Datatype::f2c(*datatype), extent);
}

void mpi_type_size_(int* datatype, int *size, int* ierr)
{
  *ierr = MPI_Type_size(Datatype::f2c(*datatype), size);
}

void mpi_error_string_(int* errorcode, char* string, int* resultlen, int* ierr){
  *ierr = MPI_Error_string(*errorcode, string, resultlen);
}

void mpi_win_fence_( int* assert,  int* win, int* ierr){
  *ierr =  MPI_Win_fence(* assert, Win::f2c(*win));
}

void mpi_win_free_( int* win, int* ierr){
  MPI_Win tmp = Win::f2c(*win);
  *ierr =  MPI_Win_free(&tmp);
  if(*ierr == MPI_SUCCESS) {
    F2C::free_f(*win);
  }
}

void mpi_win_create_( int *base, MPI_Aint* size, int* disp_unit, int* info, int* comm, int *win, int* ierr){
  MPI_Win tmp;
  *ierr =  MPI_Win_create( static_cast<void*>(base), *size, *disp_unit, Info::f2c(*info), Comm::f2c(*comm),&tmp);
 if(*ierr == MPI_SUCCESS) {
   *win = tmp->add_f();
 }
}

void mpi_win_post_(int* group, int assert, int* win, int* ierr){
  *ierr =  MPI_Win_post(Group::f2c(*group), assert, Win::f2c(*win));
}

void mpi_win_start_(int* group, int assert, int* win, int* ierr){
  *ierr =  MPI_Win_start(Group::f2c(*group), assert, Win::f2c(*win));
}

void mpi_win_complete_(int* win, int* ierr){
  *ierr =  MPI_Win_complete(Win::f2c(*win));
}

void mpi_win_wait_(int* win, int* ierr){
  *ierr =  MPI_Win_wait(Win::f2c(*win));
}

void mpi_win_set_name_ (int*  win, char * name, int* ierr, int size){
  //handle trailing blanks
  while(name[size-1]==' ')
    size--;
  while(*name==' '){//handle leading blanks
    size--;
    name++;
  }
  char* tname = xbt_new(char,size+1);
  strncpy(tname, name, size);
  tname[size]='\0';
  *ierr = MPI_Win_set_name(Win::f2c(*win), tname);
  xbt_free(tname);
}

void mpi_win_get_name_ (int*  win, char * name, int* len, int* ierr){
  *ierr = MPI_Win_get_name(Win::f2c(*win),name,len);
  if(*len>0) 
    name[*len]=' ';//blank padding, not \0
}

void mpi_info_create_( int *info, int* ierr){
  MPI_Info tmp;
  *ierr =  MPI_Info_create(&tmp);
  if(*ierr == MPI_SUCCESS) {
    *info = tmp->add_f();
  }
}

void mpi_info_set_( int *info, char *key, char *value, int* ierr, unsigned int keylen, unsigned int valuelen){
  //handle trailing blanks
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char,keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';  

  while(value[valuelen-1]==' ')
    valuelen--;
  while(*value==' '){//handle leading blanks
    valuelen--;
    value++;
  }
  char* tvalue = xbt_new(char,valuelen+1);
  strncpy(tvalue, value, valuelen);
  tvalue[valuelen]='\0'; 

  *ierr =  MPI_Info_set( Info::f2c(*info), tkey, tvalue);
  xbt_free(tkey);
  xbt_free(tvalue);
}

void mpi_info_get_ (int* info,char *key,int* valuelen, char *value, int *flag, int* ierr, unsigned int keylen ){
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }  
  char* tkey = xbt_new(char,keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_get(Info::f2c(*info),tkey,*valuelen, value, flag);
  xbt_free(tkey);
  if(*flag!=0){
    int replace=0;
    int i=0;
    for (i=0; i<*valuelen; i++){
      if(value[i]=='\0')
        replace=1;
      if(replace)
        value[i]=' ';
    }
  } 
}

void mpi_info_free_(int* info, int* ierr){
  MPI_Info tmp = Info::f2c(*info);
  *ierr =  MPI_Info_free(&tmp);
  if(*ierr == MPI_SUCCESS) {
    F2C::free_f(*info);
  }
}

void mpi_get_( int *origin_addr, int* origin_count, int* origin_datatype, int *target_rank,
    MPI_Aint* target_disp, int *target_count, int* tarsmpi_type_f2c, int* win, int* ierr){
  *ierr =  MPI_Get( static_cast<void*>(origin_addr),*origin_count, Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count,Datatype::f2c(*tarsmpi_type_f2c), Win::f2c(*win));
}

void mpi_accumulate_( int *origin_addr, int* origin_count, int* origin_datatype, int *target_rank,
    MPI_Aint* target_disp, int *target_count, int* tarsmpi_type_f2c, int* op, int* win, int* ierr){
  *ierr =  MPI_Accumulate( static_cast<void *>(origin_addr),*origin_count, Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count,Datatype::f2c(*tarsmpi_type_f2c), Op::f2c(*op), Win::f2c(*win));
}

void mpi_put_( int *origin_addr, int* origin_count, int* origin_datatype, int *target_rank,
    MPI_Aint* target_disp, int *target_count, int* tarsmpi_type_f2c, int* win, int* ierr){
  *ierr =  MPI_Put( static_cast<void *>(origin_addr),*origin_count, Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count,Datatype::f2c(*tarsmpi_type_f2c), Win::f2c(*win));
}

//following are automatically generated, and have to be checked
void mpi_finalized_ (int * flag, int* ierr){

 *ierr = MPI_Finalized(flag);
}

void mpi_init_thread_ (int* required, int *provided, int* ierr){
  smpi_init_fortran_types();
  *ierr = MPI_Init_thread(nullptr, nullptr,*required, provided);
  running_processes++;
}

void mpi_query_thread_ (int *provided, int* ierr){

 *ierr = MPI_Query_thread(provided);
}

void mpi_is_thread_main_ (int *flag, int* ierr){

 *ierr = MPI_Is_thread_main(flag);
}

void mpi_address_ (void *location, MPI_Aint * address, int* ierr){

 *ierr = MPI_Address(location, address);
}

void mpi_get_address_ (void *location, MPI_Aint * address, int* ierr){

 *ierr = MPI_Get_address(location, address);
}

void mpi_type_dup_ (int*  datatype, int* newdatatype, int* ierr){
 MPI_Datatype tmp;
 *ierr = MPI_Type_dup(Datatype::f2c(*datatype), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newdatatype = tmp->add_f();
 }
}

void mpi_type_set_name_ (int*  datatype, char * name, int* ierr, int size){
 char* tname = xbt_new(char, size+1);
 strncpy(tname, name, size);
 tname[size]='\0';
 *ierr = MPI_Type_set_name(Datatype::f2c(*datatype), tname);
 xbt_free(tname);
}

void mpi_type_get_name_ (int*  datatype, char * name, int* len, int* ierr){
 *ierr = MPI_Type_get_name(Datatype::f2c(*datatype),name,len);
  if(*len>0) 
    name[*len]=' ';
}

void mpi_type_get_attr_ (int* type, int* type_keyval, void *attribute_val, int* flag, int* ierr){

 *ierr = MPI_Type_get_attr ( Datatype::f2c(*type), *type_keyval, attribute_val,flag);
}

void mpi_type_set_attr_ (int* type, int* type_keyval, void *attribute_val, int* ierr){

 *ierr = MPI_Type_set_attr ( Datatype::f2c(*type), *type_keyval, attribute_val);
}

void mpi_type_delete_attr_ (int* type, int* type_keyval, int* ierr){

 *ierr = MPI_Type_delete_attr ( Datatype::f2c(*type),  *type_keyval);
}

void mpi_type_create_keyval_ (void* copy_fn, void*  delete_fn, int* keyval, void* extra_state, int* ierr){

 *ierr = MPI_Type_create_keyval(reinterpret_cast<MPI_Type_copy_attr_function*>(copy_fn), reinterpret_cast<MPI_Type_delete_attr_function*>(delete_fn),
                                keyval,  extra_state) ;
}

void mpi_type_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Type_free_keyval( keyval);
}

void mpi_pcontrol_ (int* level , int* ierr){
 *ierr = MPI_Pcontrol(*static_cast<const int*>(level));
}

void mpi_type_get_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr){

 *ierr = MPI_Type_get_extent(Datatype::f2c(*datatype), lb, extent);
}

void mpi_type_get_true_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr){

 *ierr = MPI_Type_get_true_extent(Datatype::f2c(*datatype), lb, extent);
}

void mpi_op_create_ (void * function, int* commute, int* op, int* ierr){
  MPI_Op tmp;
 *ierr = MPI_Op_create(reinterpret_cast<MPI_User_function*>(function),*commute, &tmp);
 if(*ierr == MPI_SUCCESS) {
   tmp->set_fortran_op();
   *op = tmp->add_f();
 }
}

void mpi_op_free_ (int* op, int* ierr){
  MPI_Op tmp=Op::f2c(*op);
  *ierr = MPI_Op_free(& tmp);
  if(*ierr == MPI_SUCCESS) {
    F2C::free_f(*op);
  }
}

void mpi_group_free_ (int* group, int* ierr){
 MPI_Group tmp=Group::f2c(*group);
 *ierr = MPI_Group_free(&tmp);
 if(*ierr == MPI_SUCCESS) {
   F2C::free_f(*group);
 }
}

void mpi_group_size_ (int* group, int *size, int* ierr){

 *ierr = MPI_Group_size(Group::f2c(*group), size);
}

void mpi_group_rank_ (int* group, int *rank, int* ierr){

 *ierr = MPI_Group_rank(Group::f2c(*group), rank);
}

void mpi_group_translate_ranks_ (int* group1, int* n, int *ranks1, int* group2, int *ranks2, int* ierr)
{

 *ierr = MPI_Group_translate_ranks(Group::f2c(*group1), *n, ranks1, Group::f2c(*group2), ranks2);
}

void mpi_group_compare_ (int* group1, int* group2, int *result, int* ierr){

 *ierr = MPI_Group_compare(Group::f2c(*group1), Group::f2c(*group2), result);
}

void mpi_group_union_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_union(Group::f2c(*group1), Group::f2c(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_group_intersection_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_intersection(Group::f2c(*group1), Group::f2c(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_group_difference_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_difference(Group::f2c(*group1), Group::f2c(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_group_excl_ (int* group, int* n, int *ranks, int* newgroup, int* ierr){
  MPI_Group tmp;
 *ierr = MPI_Group_excl(Group::f2c(*group), *n, ranks, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_group_range_incl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
  MPI_Group tmp;
 *ierr = MPI_Group_range_incl(Group::f2c(*group), *n, ranges, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_group_range_excl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
 MPI_Group tmp;
 *ierr = MPI_Group_range_excl(Group::f2c(*group), *n, ranges, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_comm_get_attr_ (int* comm, int* comm_keyval, void *attribute_val, int *flag, int* ierr){

 *ierr = MPI_Comm_get_attr (Comm::f2c(*comm), *comm_keyval, attribute_val, flag);
}

void mpi_comm_set_attr_ (int* comm, int* comm_keyval, void *attribute_val, int* ierr){

 *ierr = MPI_Comm_set_attr ( Comm::f2c(*comm), *comm_keyval, attribute_val);
}

void mpi_comm_delete_attr_ (int* comm, int* comm_keyval, int* ierr){

 *ierr = MPI_Comm_delete_attr (Comm::f2c(*comm),  *comm_keyval);
}

void mpi_comm_create_keyval_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr){

 *ierr = MPI_Comm_create_keyval(reinterpret_cast<MPI_Comm_copy_attr_function*>(copy_fn),  reinterpret_cast<MPI_Comm_delete_attr_function*>(delete_fn),
         keyval,  extra_state) ;
}

void mpi_comm_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Comm_free_keyval( keyval);
}

void mpi_comm_get_name_ (int* comm, char* name, int* len, int* ierr){
 *ierr = MPI_Comm_get_name(Comm::f2c(*comm), name, len);
  if(*len>0) 
    name[*len]=' ';
}

void mpi_comm_compare_ (int* comm1, int* comm2, int *result, int* ierr){

 *ierr = MPI_Comm_compare(Comm::f2c(*comm1), Comm::f2c(*comm2), result);
}

void mpi_comm_disconnect_ (int* comm, int* ierr){
 MPI_Comm tmp=Comm::f2c(*comm);
 *ierr = MPI_Comm_disconnect(&tmp);
 if(*ierr == MPI_SUCCESS) {
   Comm::free_f(*comm);
 }
}

void mpi_request_free_ (int* request, int* ierr){
  MPI_Request tmp=Request::f2c(*request);
 *ierr = MPI_Request_free(&tmp);
 if(*ierr == MPI_SUCCESS) {
   Request::free_f(*request);
 }
}

void mpi_sendrecv_replace_ (void *buf, int* count, int* datatype, int* dst, int* sendtag, int* src, int* recvtag,
                            int* comm, MPI_Status* status, int* ierr)
{
 *ierr = MPI_Sendrecv_replace(buf, *count, Datatype::f2c(*datatype), *dst, *sendtag, *src,
 *recvtag, Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}

void mpi_testany_ (int* count, int* requests, int *index, int *flag, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr = MPI_Testany(*count, reqs, index, flag, FORT_STATUS_IGNORE(status));
  if(*index!=MPI_UNDEFINED && reqs[*index]==MPI_REQUEST_NULL){
    Request::free_f(requests[*index]);
    requests[*index]=MPI_FORTRAN_REQUEST_NULL;
  }
  xbt_free(reqs);
}

void mpi_waitsome_ (int* incount, int* requests, int *outcount, int *indices, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *incount);
  for(i = 0; i < *incount; i++) {
    reqs[i] = Request::f2c(requests[i]);
  }
  *ierr = MPI_Waitsome(*incount, reqs, outcount, indices, status);
  for(i=0;i<*outcount;i++){
    if(reqs[indices[i]]==MPI_REQUEST_NULL){
        Request::free_f(requests[indices[i]]);
        requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
  xbt_free(reqs);
}

void mpi_reduce_local_ (void *inbuf, void *inoutbuf, int* count, int* datatype, int* op, int* ierr){

 *ierr = MPI_Reduce_local(inbuf, inoutbuf, *count, Datatype::f2c(*datatype), Op::f2c(*op));
}

void mpi_reduce_scatter_block_ (void *sendbuf, void *recvbuf, int* recvcount, int* datatype, int* op, int* comm,
                                int* ierr)
{
  sendbuf = static_cast<char *>( FORT_IN_PLACE(sendbuf));
 *ierr = MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, Datatype::f2c(*datatype), Op::f2c(*op),
                                  Comm::f2c(*comm));
}

void mpi_pack_size_ (int* incount, int* datatype, int* comm, int* size, int* ierr) {
 *ierr = MPI_Pack_size(*incount, Datatype::f2c(*datatype), Comm::f2c(*comm), size);
}

void mpi_cart_coords_ (int* comm, int* rank, int* maxdims, int* coords, int* ierr) {
 *ierr = MPI_Cart_coords(Comm::f2c(*comm), *rank, *maxdims, coords);
}

void mpi_cart_create_ (int* comm_old, int* ndims, int* dims, int* periods, int* reorder, int*  comm_cart, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Cart_create(Comm::f2c(*comm_old), *ndims, dims, periods, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_cart = tmp->add_f();
 }
}

void mpi_cart_get_ (int* comm, int* maxdims, int* dims, int* periods, int* coords, int* ierr) {
 *ierr = MPI_Cart_get(Comm::f2c(*comm), *maxdims, dims, periods, coords);
}

void mpi_cart_map_ (int* comm_old, int* ndims, int* dims, int* periods, int* newrank, int* ierr) {
 *ierr = MPI_Cart_map(Comm::f2c(*comm_old), *ndims, dims, periods, newrank);
}

void mpi_cart_rank_ (int* comm, int* coords, int* rank, int* ierr) {
 *ierr = MPI_Cart_rank(Comm::f2c(*comm), coords, rank);
}

void mpi_cart_shift_ (int* comm, int* direction, int* displ, int* source, int* dest, int* ierr) {
 *ierr = MPI_Cart_shift(Comm::f2c(*comm), *direction, *displ, source, dest);
}

void mpi_cart_sub_ (int* comm, int* remain_dims, int*  comm_new, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Cart_sub(Comm::f2c(*comm), remain_dims, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_new = tmp->add_f();
 }
}

void mpi_cartdim_get_ (int* comm, int* ndims, int* ierr) {
 *ierr = MPI_Cartdim_get(Comm::f2c(*comm), ndims);
}

void mpi_graph_create_ (int* comm_old, int* nnodes, int* index, int* edges, int* reorder, int*  comm_graph, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Graph_create(Comm::f2c(*comm_old), *nnodes, index, edges, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_graph = tmp->add_f();
 }
}

void mpi_graph_get_ (int* comm, int* maxindex, int* maxedges, int* index, int* edges, int* ierr) {
 *ierr = MPI_Graph_get(Comm::f2c(*comm), *maxindex, *maxedges, index, edges);
}

void mpi_graph_map_ (int* comm_old, int* nnodes, int* index, int* edges, int* newrank, int* ierr) {
 *ierr = MPI_Graph_map(Comm::f2c(*comm_old), *nnodes, index, edges, newrank);
}

void mpi_graph_neighbors_ (int* comm, int* rank, int* maxneighbors, int* neighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors(Comm::f2c(*comm), *rank, *maxneighbors, neighbors);
}

void mpi_graph_neighbors_count_ (int* comm, int* rank, int* nneighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors_count(Comm::f2c(*comm), *rank, nneighbors);
}

void mpi_graphdims_get_ (int* comm, int* nnodes, int* nedges, int* ierr) {
 *ierr = MPI_Graphdims_get(Comm::f2c(*comm), nnodes, nedges);
}

void mpi_topo_test_ (int* comm, int* top_type, int* ierr) {
 *ierr = MPI_Topo_test(Comm::f2c(*comm), top_type);
}

void mpi_error_class_ (int* errorcode, int* errorclass, int* ierr) {
 *ierr = MPI_Error_class(*errorcode, errorclass);
}

void mpi_errhandler_create_ (void* function, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_create(reinterpret_cast<MPI_Handler_function*>(function), static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_errhandler_free_ (void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_free(static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_errhandler_get_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_get(Comm::f2c(*comm), static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_errhandler_set_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(Comm::f2c(*comm), *static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_comm_set_errhandler_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(Comm::f2c(*comm), *static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_comm_get_errhandler_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(Comm::f2c(*comm), static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_type_contiguous_ (int* count, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_contiguous(*count, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_cancel_ (int* request, int* ierr) {
  MPI_Request tmp=Request::f2c(*request);
 *ierr = MPI_Cancel(&tmp);
}

void mpi_buffer_attach_ (void* buffer, int* size, int* ierr) {
 *ierr = MPI_Buffer_attach(buffer, *size);
}

void mpi_buffer_detach_ (void* buffer, int* size, int* ierr) {
 *ierr = MPI_Buffer_detach(buffer, size);
}

void mpi_testsome_ (int* incount, int*  requests, int* outcount, int* indices, MPI_Status*  statuses, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *incount);
  for(i = 0; i < *incount; i++) {
    reqs[i] = Request::f2c(requests[i]);
    indices[i]=0;
  }
  *ierr = MPI_Testsome(*incount, reqs, outcount, indices, FORT_STATUSES_IGNORE(statuses));
  for(i=0;i<*incount;i++){
    if(indices[i] && reqs[indices[i]]==MPI_REQUEST_NULL){
      Request::free_f(requests[indices[i]]);
      requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
  xbt_free(reqs);
}

void mpi_comm_test_inter_ (int* comm, int* flag, int* ierr) {
 *ierr = MPI_Comm_test_inter(Comm::f2c(*comm), flag);
}

void mpi_unpack_ (void* inbuf, int* insize, int* position, void* outbuf, int* outcount, int* type, int* comm,
                  int* ierr) {
 *ierr = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, Datatype::f2c(*type), Comm::f2c(*comm));
}

void mpi_pack_external_size_ (char *datarep, int* incount, int* datatype, MPI_Aint *size, int* ierr){
 *ierr = MPI_Pack_external_size(datarep, *incount, Datatype::f2c(*datatype), size);
}

void mpi_pack_external_ (char *datarep, void *inbuf, int* incount, int* datatype, void *outbuf, MPI_Aint* outcount,
                         MPI_Aint *position, int* ierr){
 *ierr = MPI_Pack_external(datarep, inbuf, *incount, Datatype::f2c(*datatype), outbuf, *outcount, position);
}

void mpi_unpack_external_ ( char *datarep, void *inbuf, MPI_Aint* insize, MPI_Aint *position, void *outbuf,
                            int* outcount, int* datatype, int* ierr){
 *ierr = MPI_Unpack_external( datarep, inbuf, *insize, position, outbuf, *outcount, Datatype::f2c(*datatype));
}

void mpi_type_hindexed_ (int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_hindexed(*count, blocklens, indices, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_hindexed_(int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed(*count, blocklens, indices, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_hindexed_block_ (int* count, int* blocklength, MPI_Aint* indices, int* old_type, int*  newtype,
                                      int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed_block(*count, *blocklength, indices, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_indexed_ (int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_indexed(*count, blocklens, indices, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_indexed_block_ (int* count, int* blocklength, int* indices,  int* old_type,  int*newtype,
                                     int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_indexed_block(*count, *blocklength, indices, Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_struct_ (int* count, int* blocklens, MPI_Aint* indices, int* old_types, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  int i=0;
  MPI_Datatype* types = static_cast<MPI_Datatype*>(xbt_malloc(*count*sizeof(MPI_Datatype)));
  for(i=0; i< *count; i++){
    types[i] = Datatype::f2c(old_types[i]);
  }
  *ierr = MPI_Type_struct(*count, blocklens, indices, types, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
  xbt_free(types);
}

void mpi_type_create_struct_(int* count, int* blocklens, MPI_Aint* indices, int*  old_types, int*  newtype, int* ierr){
  MPI_Datatype tmp;
  int i=0;
  MPI_Datatype* types = static_cast<MPI_Datatype*>(xbt_malloc(*count*sizeof(MPI_Datatype)));
  for(i=0; i< *count; i++){
    types[i] = Datatype::f2c(old_types[i]);
  }
  *ierr = MPI_Type_create_struct(*count, blocklens, indices, types, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
  xbt_free(types);
}

void mpi_ssend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr) {
 *ierr = MPI_Ssend(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm));
}

void mpi_ssend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request tmp;
 *ierr = MPI_Ssend_init(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_intercomm_create_ (int* local_comm, int *local_leader, int* peer_comm, int* remote_leader, int* tag,
                            int* comm_out, int* ierr) {
  MPI_Comm tmp;
  *ierr = MPI_Intercomm_create(Comm::f2c(*local_comm), *local_leader,Comm::f2c(*peer_comm), *remote_leader,
                               *tag, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = tmp->add_f();
  }
}

void mpi_intercomm_merge_ (int* comm, int* high, int*  comm_out, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Intercomm_merge(Comm::f2c(*comm), *high, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_out = tmp->add_f();
 }
}

void mpi_bsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int* ierr) {
 *ierr = MPI_Bsend(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm));
}

void mpi_bsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Bsend_init(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_ibsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Ibsend(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_comm_remote_group_ (int* comm, int*  group, int* ierr) {
  MPI_Group tmp;
 *ierr = MPI_Comm_remote_group(Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *group = tmp->c2f();
 }
}

void mpi_comm_remote_size_ (int* comm, int* size, int* ierr) {
 *ierr = MPI_Comm_remote_size(Comm::f2c(*comm), size);
}

void mpi_issend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Issend(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_probe_ (int* source, int* tag, int* comm, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Probe(*source, *tag, Comm::f2c(*comm), FORT_STATUS_IGNORE(status));
}

void mpi_attr_delete_ (int* comm, int* keyval, int* ierr) {
 *ierr = MPI_Attr_delete(Comm::f2c(*comm), *keyval);
}

void mpi_attr_put_ (int* comm, int* keyval, void* attr_value, int* ierr) {
 *ierr = MPI_Attr_put(Comm::f2c(*comm), *keyval, attr_value);
}

void mpi_rsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Rsend_init(buf, *count, Datatype::f2c(*datatype), *dest, *tag, Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_keyval_create_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr) {
 *ierr = MPI_Keyval_create(reinterpret_cast<MPI_Copy_function*>(copy_fn),reinterpret_cast<MPI_Delete_function*>(delete_fn), keyval, extra_state);
}

void mpi_keyval_free_ (int* keyval, int* ierr) {
 *ierr = MPI_Keyval_free(keyval);
}

void mpi_test_cancelled_ (MPI_Status*  status, int* flag, int* ierr) {
 *ierr = MPI_Test_cancelled(status, flag);
}

void mpi_pack_ (void* inbuf, int* incount, int* type, void* outbuf, int* outcount, int* position, int* comm, int* ierr) {
 *ierr = MPI_Pack(inbuf, *incount, Datatype::f2c(*type), outbuf, *outcount, position, Comm::f2c(*comm));
}

void mpi_get_elements_ (MPI_Status*  status, int* datatype, int* elements, int* ierr) {
 *ierr = MPI_Get_elements(status, Datatype::f2c(*datatype), elements);
}

void mpi_dims_create_ (int* nnodes, int* ndims, int* dims, int* ierr) {
 *ierr = MPI_Dims_create(*nnodes, *ndims, dims);
}

void mpi_iprobe_ (int* source, int* tag, int* comm, int* flag, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Iprobe(*source, *tag, Comm::f2c(*comm), flag, status);
}

void mpi_type_get_envelope_ ( int* datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner,
                              int* ierr){
 *ierr = MPI_Type_get_envelope(  Datatype::f2c(*datatype), num_integers,
 num_addresses, num_datatypes, combiner);
}

void mpi_type_get_contents_ (int* datatype, int* max_integers, int* max_addresses, int* max_datatypes,
                             int* array_of_integers, MPI_Aint* array_of_addresses,
 int* array_of_datatypes, int* ierr){
 *ierr = MPI_Type_get_contents(Datatype::f2c(*datatype), *max_integers, *max_addresses,*max_datatypes,
                               array_of_integers, array_of_addresses, reinterpret_cast<MPI_Datatype*>(array_of_datatypes));
}

void mpi_type_create_darray_ (int* size, int* rank, int* ndims, int* array_of_gsizes, int* array_of_distribs,
                              int* array_of_dargs, int* array_of_psizes,
 int* order, int* oldtype, int*newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_darray(*size, *rank, *ndims,  array_of_gsizes,
  array_of_distribs,  array_of_dargs,  array_of_psizes,
  *order,  Datatype::f2c(*oldtype), &tmp) ;
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_resized_ (int* oldtype,MPI_Aint* lb, MPI_Aint* extent, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_resized(Datatype::f2c(*oldtype),*lb, *extent, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_create_subarray_ (int* ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts,
                                int* order, int* oldtype, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_subarray(*ndims,array_of_sizes, array_of_subsizes, array_of_starts, *order,
                                   Datatype::f2c(*oldtype), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->add_f();
  }
}

void mpi_type_match_size_ (int* typeclass,int* size,int* datatype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_match_size(*typeclass,*size,&tmp);
  if(*ierr == MPI_SUCCESS) {
    *datatype = tmp->c2f();
  }
}

void mpi_alltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int* sendtypes, void *recvbuf, int *recvcnts,
                      int *rdispls, int* recvtypes, int* comm, int* ierr){
 *ierr = MPI_Alltoallw( sendbuf, sendcnts, sdispls, reinterpret_cast<MPI_Datatype*>(sendtypes), recvbuf, recvcnts, rdispls,
                        reinterpret_cast<MPI_Datatype*>(recvtypes), Comm::f2c(*comm));
}

void mpi_exscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr){
 *ierr = MPI_Exscan(sendbuf, recvbuf, *count, Datatype::f2c(*datatype), Op::f2c(*op), Comm::f2c(*comm));
}

void mpi_comm_set_name_ (int* comm, char* name, int* ierr, int size){
 char* tname = xbt_new(char, size+1);
 strncpy(tname, name, size);
 tname[size]='\0';
 *ierr = MPI_Comm_set_name (Comm::f2c(*comm), tname);
 xbt_free(tname);
}

void mpi_comm_dup_with_info_ (int* comm, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_dup_with_info(Comm::f2c(*comm),Info::f2c(*info),&tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_comm_split_type_ (int* comm, int* split_type, int* key, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_split_type(Comm::f2c(*comm), *split_type, *key, Info::f2c(*info), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_comm_set_info_ (int* comm, int* info, int* ierr){
 *ierr = MPI_Comm_set_info (Comm::f2c(*comm), Info::f2c(*info));
}

void mpi_comm_get_info_ (int* comm, int* info, int* ierr){
 MPI_Info tmp;
 *ierr = MPI_Comm_get_info (Comm::f2c(*comm), &tmp);
 if(*ierr==MPI_SUCCESS){
   *info = tmp->c2f();
 }
}

void mpi_comm_create_errhandler_ ( void *function, void *errhandler, int* ierr){
 *ierr = MPI_Comm_create_errhandler( reinterpret_cast<MPI_Comm_errhandler_fn*>(function), static_cast<MPI_Errhandler*>(errhandler));
}

void mpi_add_error_class_ ( int *errorclass, int* ierr){
 *ierr = MPI_Add_error_class( errorclass);
}

void mpi_add_error_code_ (  int* errorclass, int *errorcode, int* ierr){
 *ierr = MPI_Add_error_code(*errorclass, errorcode);
}

void mpi_add_error_string_ ( int* errorcode, char *string, int* ierr){
 *ierr = MPI_Add_error_string(*errorcode, string);
}

void mpi_comm_call_errhandler_ (int* comm,int* errorcode, int* ierr){
 *ierr = MPI_Comm_call_errhandler(Comm::f2c(*comm), *errorcode);
}

void mpi_info_dup_ (int* info, int* newinfo, int* ierr){
 MPI_Info tmp;
 *ierr = MPI_Info_dup(Info::f2c(*info), &tmp);
 if(*ierr==MPI_SUCCESS){
   *newinfo= tmp->add_f();
 }
}

void mpi_info_get_valuelen_ ( int* info, char *key, int *valuelen, int *flag, int* ierr, unsigned int keylen){
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char, keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_get_valuelen( Info::f2c(*info), tkey, valuelen, flag);
  xbt_free(tkey);
}

void mpi_info_delete_ (int* info, char *key, int* ierr, unsigned int keylen){
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char, keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_delete(Info::f2c(*info), tkey);
  xbt_free(tkey);
}

void mpi_info_get_nkeys_ ( int* info, int *nkeys, int* ierr){
 *ierr = MPI_Info_get_nkeys(  Info::f2c(*info), nkeys);
}

void mpi_info_get_nthkey_ ( int* info, int* n, char *key, int* ierr, unsigned int keylen){
  *ierr = MPI_Info_get_nthkey( Info::f2c(*info), *n, key);
  unsigned int i = 0;
  for (i=strlen(key); i<keylen; i++)
    key[i]=' ';
}

void mpi_get_version_ (int *version,int *subversion, int* ierr){
 *ierr = MPI_Get_version (version,subversion);
}

void mpi_get_library_version_ (char *version,int *len, int* ierr){
 *ierr = MPI_Get_library_version (version,len);
}

void mpi_request_get_status_ ( int* request, int *flag, MPI_Status* status, int* ierr){
 *ierr = MPI_Request_get_status( Request::f2c(*request), flag, status);
}

void mpi_grequest_start_ ( void *query_fn, void *free_fn, void *cancel_fn, void *extra_state, int*request, int* ierr){
  MPI_Request tmp;
  *ierr = MPI_Grequest_start( reinterpret_cast<MPI_Grequest_query_function*>(query_fn), reinterpret_cast<MPI_Grequest_free_function*>(free_fn),
                              reinterpret_cast<MPI_Grequest_cancel_function*>(cancel_fn), extra_state, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_grequest_complete_ ( int* request, int* ierr){
 *ierr = MPI_Grequest_complete( Request::f2c(*request));
}

void mpi_status_set_cancelled_ (MPI_Status* status,int* flag, int* ierr){
 *ierr = MPI_Status_set_cancelled(status,*flag);
}

void mpi_status_set_elements_ ( MPI_Status* status, int* datatype, int* count, int* ierr){
 *ierr = MPI_Status_set_elements( status, Datatype::f2c(*datatype), *count);
}

void mpi_comm_connect_ ( char *port_name, int* info, int* root, int* comm, int*newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_connect( port_name, *reinterpret_cast<MPI_Info*>(info), *root, Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_publish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Publish_name( service_name, *reinterpret_cast<MPI_Info*>(info), port_name);
}

void mpi_unpublish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Unpublish_name( service_name, *reinterpret_cast<MPI_Info*>(info), port_name);
}

void mpi_lookup_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Lookup_name( service_name, *reinterpret_cast<MPI_Info*>(info), port_name);
}

void mpi_comm_join_ ( int* fd, int* intercomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_join( *fd, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = tmp->add_f();
  }
}

void mpi_open_port_ ( int* info, char *port_name, int* ierr){
 *ierr = MPI_Open_port( *reinterpret_cast<MPI_Info*>(info),port_name);
}

void mpi_close_port_ ( char *port_name, int* ierr){
 *ierr = MPI_Close_port( port_name);
}

void mpi_comm_accept_ ( char *port_name, int* info, int* root, int* comm, int*newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_accept( port_name, *reinterpret_cast<MPI_Info*>(info), *root, Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->add_f();
  }
}

void mpi_comm_spawn_ ( char *command, char *argv, int* maxprocs, int* info, int* root, int* comm, int* intercomm,
                       int* array_of_errcodes, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_spawn( command, nullptr, *maxprocs, *reinterpret_cast<MPI_Info*>(info), *root, Comm::f2c(*comm), &tmp,
                          array_of_errcodes);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = tmp->add_f();
  }
}

void mpi_comm_spawn_multiple_ ( int* count, char *array_of_commands, char** array_of_argv, int* array_of_maxprocs,
                                int* array_of_info, int* root,
 int* comm, int* intercomm, int* array_of_errcodes, int* ierr){
 MPI_Comm tmp;
 *ierr = MPI_Comm_spawn_multiple(* count, &array_of_commands, &array_of_argv, array_of_maxprocs,
 reinterpret_cast<MPI_Info*>(array_of_info), *root, Comm::f2c(*comm), &tmp, array_of_errcodes);
 if(*ierr == MPI_SUCCESS) {
   *intercomm = tmp->add_f();
 }
}

void mpi_comm_get_parent_ ( int* parent, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_get_parent( &tmp);
  if(*ierr == MPI_SUCCESS) {
    *parent = tmp->c2f();
  }
}

void mpi_file_close_ ( int* file, int* ierr){
  *ierr= MPI_File_close(reinterpret_cast<MPI_File*>(*file));
}

void mpi_file_delete_ ( char* filename, int* info, int* ierr){
  *ierr= MPI_File_delete(filename, Info::f2c(*info));
}

void mpi_file_open_ ( int* comm, char* filename, int* amode, int* info, int* fh, int* ierr){
  *ierr= MPI_File_open(Comm::f2c(*comm), filename, *amode, Info::f2c(*info), reinterpret_cast<MPI_File*>(*fh));
}

void mpi_file_set_view_ ( int* fh, long long int* offset, int* etype, int* filetype, char* datarep, int* info, int* ierr){
  *ierr= MPI_File_set_view(reinterpret_cast<MPI_File>(*fh) , reinterpret_cast<MPI_Offset>(*offset), Datatype::f2c(*etype), Datatype::f2c(*filetype), datarep, Info::f2c(*info));
}

void mpi_file_read_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read(reinterpret_cast<MPI_File>(*fh), buf, *count, Datatype::f2c(*datatype), status);
}

void mpi_file_write_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write(reinterpret_cast<MPI_File>(*fh), buf, *count, Datatype::f2c(*datatype), status);
}

} // extern "C"
