/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <limits.h>
#include <stdio.h>

#include "private.h"
#include "xbt.h"

extern int xargc;
extern char** xargv;

static xbt_dynar_t comm_lookup = NULL;
static xbt_dynar_t group_lookup = NULL;
static xbt_dict_t request_lookup = NULL;
static xbt_dynar_t datatype_lookup = NULL;
static xbt_dynar_t op_lookup = NULL;

#define KEY_SIZE (sizeof(int) * 2 + 1)

static int new_comm(MPI_Comm comm) {
  xbt_dynar_push(comm_lookup, &comm);
  return (int)xbt_dynar_length(comm_lookup) - 1;
}

static void free_comm(int comm) {
  xbt_dynar_remove_at(comm_lookup, comm, NULL);
}

static MPI_Comm get_comm(int comm) {
  if(comm == -2) {
    return MPI_COMM_SELF;
  } else if(comm_lookup && comm >= 0 && comm < (int)xbt_dynar_length(comm_lookup)) {
    return *(MPI_Comm*)xbt_dynar_get_ptr(comm_lookup, comm);
  }
  return MPI_COMM_NULL;
}

static int new_group(MPI_Group group) {
  xbt_dynar_push(group_lookup, &group);
  return (int)xbt_dynar_length(group_lookup) - 1;
}

static MPI_Group get_group(int group) {
  if(group == -2) {
    return MPI_GROUP_EMPTY;
  } else if(group_lookup && group >= 0 && group < (int)xbt_dynar_length(group_lookup)) {
    return *(MPI_Group*)xbt_dynar_get_ptr(group_lookup, group);
  }
  return MPI_COMM_NULL;
}

static char* get_key(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x", id);
  return key;
}

static int new_request(MPI_Request req) {
  static int request_id = INT_MIN;
  char key[KEY_SIZE];

  xbt_dict_set(request_lookup, get_key(key, request_id), req, NULL);
  return request_id++;
}

static MPI_Request find_request(int req) {
  char key[KEY_SIZE];
   
  return (MPI_Request)xbt_dict_get(request_lookup, get_key(key, req));
}

static int new_datatype(MPI_Datatype datatype) {
  xbt_dynar_push(datatype_lookup, &datatype);
  return (int)xbt_dynar_length(datatype_lookup) - 1;
}

static MPI_Datatype get_datatype(int datatype) {
  return datatype >= 0
         ? *(MPI_Datatype*)xbt_dynar_get_ptr(datatype_lookup, datatype)
         : MPI_DATATYPE_NULL;
}

static void free_datatype(int datatype) {
  xbt_dynar_remove_at(datatype_lookup, datatype, NULL);
}

static int new_op(MPI_Op op) {
  xbt_dynar_push(op_lookup, &op);
  return (int)xbt_dynar_length(op_lookup) - 1;
}

static MPI_Op get_op(int op) {
   return op >= 0
          ? *(MPI_Op*)xbt_dynar_get_ptr(op_lookup, op)
          : MPI_OP_NULL;
}

void mpi_init_(int* ierr) {
   comm_lookup = xbt_dynar_new(sizeof(MPI_Comm), NULL);
   new_comm(MPI_COMM_WORLD);
   group_lookup = xbt_dynar_new(sizeof(MPI_Group), NULL);

   request_lookup = xbt_dict_new_homogeneous(NULL);

   datatype_lookup = xbt_dynar_new(sizeof(MPI_Datatype), NULL);
   new_datatype(MPI_BYTE);
   new_datatype(MPI_CHAR);
   new_datatype(MPI_INT);
   new_datatype(MPI_INT);
   new_datatype(MPI_INT8_T);
   new_datatype(MPI_INT16_T);
   new_datatype(MPI_INT32_T);
   new_datatype(MPI_INT64_T);
   new_datatype(MPI_FLOAT);
   new_datatype(MPI_FLOAT);
   new_datatype(MPI_DOUBLE);
   new_datatype(MPI_DOUBLE);
   new_datatype(MPI_C_FLOAT_COMPLEX);
   new_datatype(MPI_C_DOUBLE_COMPLEX);
   new_datatype(MPI_2INT);
   new_datatype(MPI_UINT8_T);
   new_datatype(MPI_UINT16_T);
   new_datatype(MPI_UINT32_T);
   new_datatype(MPI_UINT64_T);
   new_datatype(MPI_2FLOAT);
   new_datatype(MPI_2DOUBLE);


   op_lookup = xbt_dynar_new(sizeof(MPI_Op), NULL);
   new_op(MPI_MAX);
   new_op(MPI_MIN);
   new_op(MPI_MAXLOC);
   new_op(MPI_MINLOC);
   new_op(MPI_SUM);
   new_op(MPI_PROD);
   new_op(MPI_LAND);
   new_op(MPI_LOR);
   new_op(MPI_LXOR);
   new_op(MPI_BAND);
   new_op(MPI_BOR);
   new_op(MPI_BXOR);

   /* smpif2c is responsible for generating a call with the final arguments */
   *ierr = MPI_Init(NULL, NULL);
}

void mpi_finalize_(int* ierr) {
   *ierr = MPI_Finalize();
   xbt_dynar_free(&op_lookup);
   op_lookup = NULL;
   xbt_dynar_free(&datatype_lookup);
   datatype_lookup = NULL;
   xbt_dict_free(&request_lookup);
   request_lookup = NULL;
   xbt_dynar_free(&comm_lookup);
   comm_lookup = NULL;
}

void mpi_abort_(int* comm, int* errorcode, int* ierr) {
  *ierr = MPI_Abort(get_comm(*comm), *errorcode);
}

void mpi_comm_rank_(int* comm, int* rank, int* ierr) {
   *ierr = MPI_Comm_rank(get_comm(*comm), rank);
}

void mpi_comm_size_(int* comm, int* size, int* ierr) {
   *ierr = MPI_Comm_size(get_comm(*comm), size);
}

double mpi_wtime_(void) {
   return MPI_Wtime();
}

double mpi_wtick_(void) {
  return MPI_Wtick();
}

void mpi_comm_dup_(int* comm, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_dup(get_comm(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_comm_create_(int* comm, int* group, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_create(get_comm(*comm),get_group(*group), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}


void mpi_comm_free_(int* comm, int* ierr) {
  MPI_Comm tmp = get_comm(*comm);

  *ierr = MPI_Comm_free(&tmp);

  if(*ierr == MPI_SUCCESS) {
    free_comm(*comm);
  }
}

void mpi_comm_split_(int* comm, int* color, int* key, int* comm_out, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_split(get_comm(*comm), *color, *key, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = new_comm(tmp);
  }
}

void mpi_group_incl_(int* group, int* n, int* ranks, int* group_out, int* ierr) {
  MPI_Group tmp;

  *ierr = MPI_Group_incl(get_group(*group), *n, ranks, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = new_group(tmp);
  }
}

void mpi_comm_group_(int* comm, int* group_out,  int* ierr) {
  MPI_Group tmp;

  *ierr = MPI_Comm_group(get_comm(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = new_group(tmp);
  }
}


void mpi_initialized_(int* flag, int* ierr){
  *ierr = MPI_Initialized(flag);
}

void mpi_send_init_(void *buf, int* count, int* datatype, int* dst, int* tag,
                     int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Send_init(buf, *count, get_datatype(*datatype), *dst, *tag,
                        get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_isend_(void *buf, int* count, int* datatype, int* dst,
                 int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Isend(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_irsend_(void *buf, int* count, int* datatype, int* dst,
                 int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Irsend(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_send_(void* buf, int* count, int* datatype, int* dst,
                int* tag, int* comm, int* ierr) {
   *ierr = MPI_Send(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm));
}

void mpi_rsend_(void* buf, int* count, int* datatype, int* dst,
                int* tag, int* comm, int* ierr) {
   *ierr = MPI_Rsend(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm));
}

void mpi_sendrecv_(void* sendbuf, int* sendcount, int* sendtype, int* dst,
                int* sendtag, void *recvbuf, int* recvcount,
                int* recvtype, int* src, int* recvtag,
                int* comm, MPI_Status* status, int* ierr) {
   *ierr = MPI_Sendrecv(sendbuf, *sendcount, get_datatype(*sendtype), *dst,
       *sendtag, recvbuf, *recvcount,get_datatype(*recvtype), *src, *recvtag,
       get_comm(*comm), status);
}

void mpi_recv_init_(void *buf, int* count, int* datatype, int* src, int* tag,
                     int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Recv_init(buf, *count, get_datatype(*datatype), *src, *tag,
                        get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_irecv_(void *buf, int* count, int* datatype, int* src, int* tag,
                 int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Irecv(buf, *count, get_datatype(*datatype), *src, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_recv_(void* buf, int* count, int* datatype, int* src,
                int* tag, int* comm, MPI_Status* status, int* ierr) {
   *ierr = MPI_Recv(buf, *count, get_datatype(*datatype), *src, *tag,
                    get_comm(*comm), status);
}

void mpi_start_(int* request, int* ierr) {
  MPI_Request req = find_request(*request);

  *ierr = MPI_Start(&req);
}

void mpi_startall_(int* count, int* requests, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Startall(*count, reqs);
  free(reqs);
}

void mpi_wait_(int* request, MPI_Status* status, int* ierr) {
   MPI_Request req = find_request(*request);
   
   *ierr = MPI_Wait(&req, status);
}

void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitany(*count, reqs, index, status);
  free(reqs);
}

void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitall(*count, reqs, status);
  free(reqs);
}

void mpi_barrier_(int* comm, int* ierr) {
  *ierr = MPI_Barrier(get_comm(*comm));
}

void mpi_bcast_(void *buf, int* count, int* datatype, int* root, int* comm, int* ierr) {
  *ierr = MPI_Bcast(buf, *count, get_datatype(*datatype), *root, get_comm(*comm));
}

void mpi_reduce_(void* sendbuf, void* recvbuf, int* count,
                  int* datatype, int* op, int* root, int* comm, int* ierr) {
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count,
                     get_datatype(*datatype), get_op(*op), *root, get_comm(*comm));
}

void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype,
                     int* op, int* comm, int* ierr) {
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, get_datatype(*datatype),
                        get_op(*op), get_comm(*comm));
}

void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype,
                     int* op, int* comm, int* ierr) {
  *ierr = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, get_datatype(*datatype),
                        get_op(*op), get_comm(*comm));
}

void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, 
                   int* root, int* comm, int* ierr) {
  *ierr = MPI_Scatter(sendbuf, *sendcount, get_datatype(*sendtype),
                      recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}


void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr) {
  *ierr = MPI_Scatterv(sendbuf, sendcounts, displs, get_datatype(*sendtype),
                      recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr) {
  *ierr = MPI_Gather(sendbuf, *sendcount, get_datatype(*sendtype),
                     recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype,
                  int* root, int* comm, int* ierr) {
  *ierr = MPI_Gatherv(sendbuf, *sendcount, get_datatype(*sendtype),
                     recvbuf, recvcounts, displs, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr) {
  *ierr = MPI_Allgather(sendbuf, *sendcount, get_datatype(*sendtype),
                        recvbuf, *recvcount, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcounts,int* displs, int* recvtype,
                     int* comm, int* ierr) {
  *ierr = MPI_Allgatherv(sendbuf, *sendcount, get_datatype(*sendtype),
                        recvbuf, recvcounts, displs, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_scan_(void* sendbuf, void* recvbuf, int* count, int* datatype,
                int* op, int* comm, int* ierr) {
  *ierr = MPI_Scan(sendbuf, recvbuf, *count, get_datatype(*datatype),
                   get_op(*op), get_comm(*comm));
}

void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoall(sendbuf, *sendcount, get_datatype(*sendtype),
                       recvbuf, *recvcount, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoallv(sendbuf, sendcounts, senddisps, get_datatype(*sendtype),
                       recvbuf, recvcounts, recvdisps, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_test_ (int * request, int *flag, MPI_Status * status, int* ierr){
  MPI_Request req = find_request(*request);
  *ierr= MPI_Test(&req, flag, status);
}


void mpi_testall_ (int* count, int * requests,  int *flag, MPI_Status * statuses, int* ierr){
  MPI_Request* reqs;
  int i;
  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr= MPI_Testall(*count, reqs, flag, statuses);
}


void mpi_get_processor_name_(char *name, int *resultlen, int* ierr){
  *ierr = MPI_Get_processor_name(name, resultlen);
}

void mpi_get_count_(MPI_Status * status, int* datatype, int *count, int* ierr){
  *ierr = MPI_Get_count(status, get_datatype(*datatype), count);
}

void mpi_attr_get_(int* comm, int* keyval, void* attr_value, int* flag, int* ierr ){
  *ierr = MPI_Attr_get(get_comm(*comm), *keyval, attr_value, flag);
}

void mpi_type_extent_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(get_datatype(*datatype),  extent);
}

void mpi_type_commit_(int* datatype,  int* ierr){
  MPI_Datatype tmp= get_datatype(*datatype);
  *ierr= MPI_Type_commit(&tmp);
}

void mpi_type_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_vector(*count, *blocklen, *stride, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_vector(*count, *blocklen, *stride, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector (*count, *blocklen, *stride, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector(*count, *blocklen, *stride, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_free_(int* datatype, int* ierr){
  MPI_Datatype tmp= get_datatype(*datatype);
  *ierr= MPI_Type_free (&tmp);
  if(*ierr == MPI_SUCCESS) {
    free_datatype(*datatype);
  }
}

void mpi_type_ub_(int* datatype, MPI_Aint * disp, int* ierr){
  *ierr= MPI_Type_ub(get_datatype(*datatype), disp);
}

void mpi_type_lb_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(get_datatype(*datatype), extent);
}

void mpi_type_size_(int* datatype, int *size, int* ierr)
{
  *ierr = MPI_Type_size(get_datatype(*datatype), size);
}

void mpi_error_string_(int* errorcode, char* string, int* resultlen, int* ierr){
  *ierr = MPI_Error_string(*errorcode, string, resultlen);
}

void mpi_win_fence_( int* assert,  int* win, int* ierr){
  *ierr =  MPI_Win_fence(* assert, *(MPI_Win*)win);
}

void mpi_win_free_( int* win, int* ierr){
  *ierr =  MPI_Win_free(  (MPI_Win*)win);
}

void mpi_win_create_( int *base, MPI_Aint* size, int* disp_unit, int* info, int* comm, int *win, int* ierr){
  *ierr =  MPI_Win_create( (void*)base, *size, *disp_unit, *(MPI_Info*)info, get_comm(*comm),(MPI_Win*)win);
}

void mpi_info_create_( int *info, int* ierr){
  *ierr =  MPI_Info_create( (MPI_Info *)info);
}

void mpi_info_set_( int *info, char *key, char *value, int* ierr){
  *ierr =  MPI_Info_set( (MPI_Info *)info, key, value);
}

void mpi_info_free_(int* info, int* ierr){
  *ierr =  MPI_Info_free((MPI_Info *) info);
}

void mpi_get_( int *origin_addr, int* origin_count, int* origin_datatype, int *target_rank,
    MPI_Aint* target_disp, int *target_count, int* target_datatype, int* win, int* ierr){
  *ierr =  MPI_Get( (void*)origin_addr,*origin_count, get_datatype(*origin_datatype),*target_rank,
      *target_disp, *target_count,get_datatype(*target_datatype), *(MPI_Win *)win);
}
