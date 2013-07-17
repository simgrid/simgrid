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

static xbt_dict_t comm_lookup = NULL;
static xbt_dict_t group_lookup = NULL;
static xbt_dict_t request_lookup = NULL;
static xbt_dict_t datatype_lookup = NULL;
static xbt_dict_t op_lookup = NULL;
static int running_processes = 0;



/* Convert between Fortran and C MPI_BOTTOM */
#define F2C_BOTTOM(addr)    ((addr!=MPI_IN_PLACE && *(int*)addr == MPI_FORTRAN_BOTTOM) ? MPI_BOTTOM : (addr))
#define F2C_IN_PLACE(addr)  ((addr!=MPI_BOTTOM &&*(int*)addr == MPI_FORTRAN_IN_PLACE) ? MPI_IN_PLACE : (addr))
#define F2C_STATUS_IGNORE(addr) ((*(int*)addr == MPI_FORTRAN_STATUS_IGNORE) ? MPI_STATUS_IGNORE : (addr))
#define F2C_STATUSES_IGNORE(addr) ((*(int*)addr == MPI_FORTRAN_STATUSES_IGNORE) ? MPI_STATUSES_IGNORE : (addr))

#define KEY_SIZE (sizeof(int) * 2 + 1)


static char* get_key(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x",id);
  return key;
}
static char* get_key_id(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x_%d",id, smpi_process_index());
  return key;
}

static int new_comm(MPI_Comm comm) {
  static int comm_id = 0;
  char key[KEY_SIZE];
  xbt_dict_set(comm_lookup, comm==MPI_COMM_WORLD? get_key(key, comm_id) : get_key_id(key, comm_id), comm, NULL);
  comm_id++;
  return comm_id-1;
}

static void free_comm(int comm) {
  char key[KEY_SIZE];
  xbt_dict_remove(comm_lookup, comm==0? get_key(key, comm) : get_key_id(key, comm));
}

static MPI_Comm get_comm(int comm) {
  if(comm == -2) {
    return MPI_COMM_SELF;
  }else if(comm==0){
    return MPI_COMM_WORLD;
  }     else if(comm_lookup && comm >= 0) {

      char key[KEY_SIZE];
      MPI_Comm tmp =  (MPI_Comm)xbt_dict_get_or_null(comm_lookup,get_key_id(key, comm));
      return tmp != NULL ? tmp : MPI_COMM_NULL ;
  }
  return MPI_COMM_NULL;
}

static int new_group(MPI_Group group) {
  static int group_id = 0;
  char key[KEY_SIZE];
  xbt_dict_set(group_lookup, get_key(key, group_id), group, NULL);
  group_id++;
  return group_id-1;
}

static MPI_Group get_group(int group) {
  if(group == -2) {
    return MPI_GROUP_EMPTY;
  } else if(group_lookup && group >= 0) {
    char key[KEY_SIZE];
    return (MPI_Group)xbt_dict_get_or_null(group_lookup, get_key(key, group));
  }
  return MPI_GROUP_NULL;
}

static void free_group(int group) {
  char key[KEY_SIZE];
  xbt_dict_remove(group_lookup, get_key(key, group));
}



static int new_request(MPI_Request req) {
  static int request_id = INT_MIN;
  char key[KEY_SIZE];
  xbt_dict_set(request_lookup, get_key_id(key, request_id), req, NULL);
  request_id++;
  return request_id-1;
}

static MPI_Request find_request(int req) {
  char key[KEY_SIZE];
  if(req==MPI_FORTRAN_REQUEST_NULL)return MPI_REQUEST_NULL;
  return (MPI_Request)xbt_dict_get(request_lookup, get_key_id(key, req));
}

static void free_request(int request) {
  char key[KEY_SIZE];
  if(request!=MPI_FORTRAN_REQUEST_NULL)
  xbt_dict_remove(request_lookup, get_key_id(key, request));
}

static int new_datatype(MPI_Datatype datatype) {
  static int datatype_id = 0;
  char key[KEY_SIZE];
  xbt_dict_set(datatype_lookup, get_key(key, datatype_id), datatype, NULL);
  datatype_id++;
  return datatype_id-1;
}

static MPI_Datatype get_datatype(int datatype) {
  char key[KEY_SIZE];
  return datatype >= 0
         ? (MPI_Datatype)xbt_dict_get_or_null(datatype_lookup, get_key(key, datatype))
         : MPI_DATATYPE_NULL;
}

static void free_datatype(int datatype) {
  char key[KEY_SIZE];
  xbt_dict_remove(datatype_lookup, get_key(key, datatype));
}

static int new_op(MPI_Op op) {
  static int op_id = 0;
  char key[KEY_SIZE];
  xbt_dict_set(op_lookup, get_key(key, op_id), op, NULL);
  op_id++;
  return op_id-1;
}

static MPI_Op get_op(int op) {
  char key[KEY_SIZE];
   return op >= 0
          ? (MPI_Op)xbt_dict_get_or_null(op_lookup,  get_key(key, op))
          : MPI_OP_NULL;
}

static void free_op(int op) {
  char key[KEY_SIZE];
  xbt_dict_remove(op_lookup, get_key(key, op));
}

void mpi_init_(int* ierr) {
   if(!comm_lookup){
     comm_lookup = xbt_dict_new_homogeneous(NULL);
     new_comm(MPI_COMM_WORLD);
     group_lookup = xbt_dict_new_homogeneous(NULL);

     request_lookup = xbt_dict_new_homogeneous(NULL);

     datatype_lookup = xbt_dict_new_homogeneous(NULL);
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
     new_datatype(MPI_DOUBLE);
     new_datatype(MPI_DOUBLE);
     new_datatype(MPI_INT);
     new_datatype(MPI_DATATYPE_NULL);
     new_datatype(MPI_DATATYPE_NULL);
     new_datatype(MPI_DATATYPE_NULL);
     new_datatype(MPI_DATATYPE_NULL);
     op_lookup = xbt_dict_new_homogeneous(NULL);
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
   }
   /* smpif2c is responsible for generating a call with the final arguments */
   *ierr = MPI_Init(NULL, NULL);
   running_processes++;
}

void mpi_finalize_(int* ierr) {
   *ierr = MPI_Finalize();
   running_processes--;
   if(running_processes==0){
     xbt_dict_free(&op_lookup);
     op_lookup = NULL;
     xbt_dict_free(&datatype_lookup);
     datatype_lookup = NULL;
     xbt_dict_free(&request_lookup);
     request_lookup = NULL;
     xbt_dict_free(&comm_lookup);
     comm_lookup = NULL;
   }
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
  buf = (char *) F2C_BOTTOM(buf);
  *ierr = MPI_Isend(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_irsend_(void *buf, int* count, int* datatype, int* dst,
                 int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;
  buf = (char *) F2C_BOTTOM(buf);
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
       get_comm(*comm), F2C_STATUS_IGNORE(status));
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
  buf = (char *) F2C_BOTTOM(buf);
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
   
   *ierr = MPI_Wait(&req, F2C_STATUS_IGNORE(status));
   if(req==MPI_REQUEST_NULL){
     free_request(*request);
     *request=MPI_FORTRAN_REQUEST_NULL;
   }
}

void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitany(*count, reqs, index, status);
  if(reqs[*index]==MPI_REQUEST_NULL){
      free_request(requests[*index]);
      requests[*index]=MPI_FORTRAN_REQUEST_NULL;
  }
  free(reqs);
}

void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitall(*count, reqs, F2C_STATUSES_IGNORE(status));
  for(i = 0; i < *count; i++) {
      if(reqs[i]==MPI_REQUEST_NULL){
          free_request(requests[i]);
          requests[i]=MPI_FORTRAN_REQUEST_NULL;
      }
  }

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
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  sendbuf = (char *) F2C_BOTTOM(sendbuf);
  recvbuf = (char *) F2C_BOTTOM(recvbuf);
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count,
                     get_datatype(*datatype), get_op(*op), *root, get_comm(*comm));
}

void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype,
                     int* op, int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, get_datatype(*datatype),
                        get_op(*op), get_comm(*comm));
}

void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype,
                     int* op, int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  *ierr = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, get_datatype(*datatype),
                        get_op(*op), get_comm(*comm));
}

void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, 
                   int* root, int* comm, int* ierr) {
  recvbuf = (char *) F2C_IN_PLACE(recvbuf);
  *ierr = MPI_Scatter(sendbuf, *sendcount, get_datatype(*sendtype),
                      recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}


void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr) {
  recvbuf = (char *) F2C_IN_PLACE(recvbuf);
  *ierr = MPI_Scatterv(sendbuf, sendcounts, displs, get_datatype(*sendtype),
                      recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  sendbuf = (char *) F2C_BOTTOM(sendbuf);
  recvbuf = (char *) F2C_BOTTOM(recvbuf);
  *ierr = MPI_Gather(sendbuf, *sendcount, get_datatype(*sendtype),
                     recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype,
                  int* root, int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  sendbuf = (char *) F2C_BOTTOM(sendbuf);
  recvbuf = (char *) F2C_BOTTOM(recvbuf);
  *ierr = MPI_Gatherv(sendbuf, *sendcount, get_datatype(*sendtype),
                     recvbuf, recvcounts, displs, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
  *ierr = MPI_Allgather(sendbuf, *sendcount, get_datatype(*sendtype),
                        recvbuf, *recvcount, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcounts,int* displs, int* recvtype,
                     int* comm, int* ierr) {
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
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
  *ierr= MPI_Test(&req, flag, F2C_STATUS_IGNORE(status));
  if(req==MPI_REQUEST_NULL){
      free_request(*request);
      *request=MPI_FORTRAN_REQUEST_NULL;
  }
}


void mpi_testall_ (int* count, int * requests,  int *flag, MPI_Status * statuses, int* ierr){
  MPI_Request* reqs;
  int i;
  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr= MPI_Testall(*count, reqs, flag, F2C_STATUSES_IGNORE(statuses));
  for(i = 0; i < *count; i++) {
    if(reqs[i]==MPI_REQUEST_NULL){
        free_request(requests[i]);
        requests[i]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
}


void mpi_get_processor_name_(char *name, int *resultlen, int* ierr){
  *ierr = MPI_Get_processor_name(name, resultlen);
}

void mpi_get_count_(MPI_Status * status, int* datatype, int *count, int* ierr){
  *ierr = MPI_Get_count(F2C_STATUS_IGNORE(status), get_datatype(*datatype), count);
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
  *ierr =  MPI_Info_set( *(MPI_Info *)info, key, value);
}

void mpi_info_free_(int* info, int* ierr){
  *ierr =  MPI_Info_free((MPI_Info *) info);
}

void mpi_get_( int *origin_addr, int* origin_count, int* origin_datatype, int *target_rank,
    MPI_Aint* target_disp, int *target_count, int* target_datatype, int* win, int* ierr){
  *ierr =  MPI_Get( (void*)origin_addr,*origin_count, get_datatype(*origin_datatype),*target_rank,
      *target_disp, *target_count,get_datatype(*target_datatype), *(MPI_Win *)win);
}


//following are automatically generated, and have to be checked
void mpi_finalized_ (int * flag, int* ierr){

 *ierr = MPI_Finalized(flag);
}

void mpi_init_thread_ (int* required, int *provided, int* ierr){
  if(!comm_lookup){
    comm_lookup = xbt_dict_new_homogeneous(NULL);
    new_comm(MPI_COMM_WORLD);
    group_lookup = xbt_dict_new_homogeneous(NULL);

    request_lookup = xbt_dict_new_homogeneous(NULL);

    datatype_lookup = xbt_dict_new_homogeneous(NULL);
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

    op_lookup = xbt_dict_new_homogeneous( NULL);
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
  }
  /* smpif2c is responsible for generating a call with the final arguments */
 *ierr = MPI_Init_thread(NULL, NULL,*required, provided);
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
 *ierr = MPI_Type_dup(get_datatype(*datatype), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newdatatype = new_datatype(tmp);
 }
}

void mpi_type_set_name_ (int*  datatype, char * name, int* ierr){

 *ierr = MPI_Type_set_name(get_datatype(*datatype), name);
}

void mpi_type_get_name_ (int*  datatype, char * name, int* len, int* ierr){

 *ierr = MPI_Type_get_name(get_datatype(*datatype),name,len);
}

void mpi_type_get_attr_ (int* type, int* type_keyval, void *attribute_val, int* flag, int* ierr){

 *ierr = MPI_Type_get_attr ( get_datatype(*type), *type_keyval, attribute_val,flag);
}

void mpi_type_set_attr_ (int* type, int* type_keyval, void *attribute_val, int* ierr){

 *ierr = MPI_Type_set_attr ( get_datatype(*type), *type_keyval, attribute_val);
}

void mpi_type_delete_attr_ (int* type, int* type_keyval, int* ierr){

 *ierr = MPI_Type_delete_attr ( get_datatype(*type),  *type_keyval);
}

void mpi_type_create_keyval_ (void* copy_fn, void*  delete_fn, int* keyval, void* extra_state, int* ierr){

 *ierr = MPI_Type_create_keyval((MPI_Type_copy_attr_function*)copy_fn, (MPI_Type_delete_attr_function*) delete_fn,  keyval,  extra_state) ;
}

void mpi_type_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Type_free_keyval( keyval);
}

void mpi_pcontrol_ (int* level , int* ierr){
 *ierr = MPI_Pcontrol(*(const int*)level);
}

void mpi_type_get_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr){

 *ierr = MPI_Type_get_extent(get_datatype(*datatype), lb, extent);
}

void mpi_type_get_true_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr){

 *ierr = MPI_Type_get_true_extent(get_datatype(*datatype), lb, extent);
}

void mpi_op_create_ (void * function, int* commute, int* op, int* ierr){
  MPI_Op tmp;
 *ierr = MPI_Op_create((MPI_User_function*)function,* commute, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *op = new_op(tmp);
 }
}

void mpi_op_free_ (int* op, int* ierr){
  MPI_Op tmp=get_op(*op);
  *ierr = MPI_Op_free(& tmp);
  if(*ierr == MPI_SUCCESS) {
    free_op(*op);
  }
}

void mpi_group_free_ (int* group, int* ierr){
 MPI_Group tmp=get_group(*group);
 *ierr = MPI_Group_free(&tmp);
 if(*ierr == MPI_SUCCESS) {
   free_group(*group);
 }
}

void mpi_group_size_ (int* group, int *size, int* ierr){

 *ierr = MPI_Group_size(get_group(*group), size);
}

void mpi_group_rank_ (int* group, int *rank, int* ierr){

 *ierr = MPI_Group_rank(get_group(*group), rank);
}

void mpi_group_translate_ranks_ (int* group1, int* n, int *ranks1, int* group2, int *ranks2, int* ierr)
{

 *ierr = MPI_Group_translate_ranks(get_group(*group1), *n, ranks1, get_group(*group2), ranks2);
}

void mpi_group_compare_ (int* group1, int* group2, int *result, int* ierr){

 *ierr = MPI_Group_compare(get_group(*group1), get_group(*group2), result);
}

void mpi_group_union_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_union(get_group(*group1), get_group(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_group_intersection_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_intersection(get_group(*group1), get_group(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_group_difference_ (int* group1, int* group2, int* newgroup, int* ierr){
 MPI_Group tmp;
 *ierr = MPI_Group_difference(get_group(*group1), get_group(*group2), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_group_excl_ (int* group, int* n, int *ranks, int* newgroup, int* ierr){
  MPI_Group tmp;
 *ierr = MPI_Group_excl(get_group(*group), *n, ranks, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_group_range_incl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
  MPI_Group tmp;
 *ierr = MPI_Group_range_incl(get_group(*group), *n, ranges, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_group_range_excl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
 MPI_Group tmp;
 *ierr = MPI_Group_range_excl(get_group(*group), *n, ranges, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = new_group(tmp);
 }
}

void mpi_comm_get_attr_ (int* comm, int* comm_keyval, void *attribute_val, int *flag, int* ierr){

 *ierr = MPI_Comm_get_attr (get_comm(*comm), *comm_keyval, attribute_val, flag);
}

void mpi_comm_set_attr_ (int* comm, int* comm_keyval, void *attribute_val, int* ierr){

 *ierr = MPI_Comm_set_attr ( get_comm(*comm), *comm_keyval, attribute_val);
}

void mpi_comm_delete_attr_ (int* comm, int* comm_keyval, int* ierr){

 *ierr = MPI_Comm_delete_attr (get_comm(*comm),  *comm_keyval);
}

void mpi_comm_create_keyval_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr){

 *ierr = MPI_Comm_create_keyval((MPI_Comm_copy_attr_function*)copy_fn,  (MPI_Comm_delete_attr_function*)delete_fn,  keyval,  extra_state) ;
}

void mpi_comm_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Comm_free_keyval( keyval);
}

void mpi_comm_get_name_ (int* comm, char* name, int* len, int* ierr){

 *ierr = MPI_Comm_get_name(get_comm(*comm), name, len);
}

void mpi_comm_compare_ (int* comm1, int* comm2, int *result, int* ierr){

 *ierr = MPI_Comm_compare(get_comm(*comm1), get_comm(*comm2), result);
}

void mpi_comm_disconnect_ (int* comm, int* ierr){
 MPI_Comm tmp=get_comm(*comm);
 *ierr = MPI_Comm_disconnect(&tmp);
 if(*ierr == MPI_SUCCESS) {
   free_comm(*comm);
 }
}

void mpi_request_free_ (int* request, int* ierr){
  MPI_Request tmp=find_request(*request);
 *ierr = MPI_Request_free(&tmp);
 if(*ierr == MPI_SUCCESS) {
   free_request(*request);
 }
}

void mpi_sendrecv_replace_ (void *buf, int* count, int* datatype, int* dst, int* sendtag, int* src, int* recvtag,
 int* comm, MPI_Status* status, int* ierr)
{

 *ierr = MPI_Sendrecv_replace(buf, *count, get_datatype(*datatype), *dst, *sendtag, *src,
 *recvtag, get_comm(*comm), F2C_STATUS_IGNORE(status));
}

void mpi_testany_ (int* count, int* requests, int *index, int *flag, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Testany(*count, reqs, index, flag, F2C_STATUS_IGNORE(status));
  if(*index!=MPI_UNDEFINED)
  if(reqs[*index]==MPI_REQUEST_NULL){
    free_request(requests[*index]);
    requests[*index]=MPI_FORTRAN_REQUEST_NULL;
  }
  free(reqs);
}

void mpi_waitsome_ (int* incount, int* requests, int *outcount, int *indices, MPI_Status* status, int* ierr)
{
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *incount);
  for(i = 0; i < *incount; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitsome(*incount, reqs, outcount, indices, status);
  for(i=0;i<*outcount;i++){
    if(reqs[indices[i]]==MPI_REQUEST_NULL){
        free_request(requests[indices[i]]);
        requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
    }
  }
  free(reqs);
}

void mpi_reduce_local_ (void *inbuf, void *inoutbuf, int* count, int* datatype, int* op, int* ierr){

 *ierr = MPI_Reduce_local(inbuf, inoutbuf, *count, get_datatype(*datatype), get_op(*op));
}

void mpi_reduce_scatter_block_ (void *sendbuf, void *recvbuf, int* recvcount, int* datatype, int* op, int* comm, int* ierr)
{
  sendbuf = (char *) F2C_IN_PLACE(sendbuf);
 *ierr = MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, get_datatype(*datatype), get_op(*op), get_comm(*comm));
}

void mpi_pack_size_ (int* incount, int* datatype, int* comm, int* size, int* ierr) {
 *ierr = MPI_Pack_size(*incount, get_datatype(*datatype), get_comm(*comm), size);
}

void mpi_cart_coords_ (int* comm, int* rank, int* maxdims, int* coords, int* ierr) {
 *ierr = MPI_Cart_coords(get_comm(*comm), *rank, *maxdims, coords);
}

void mpi_cart_create_ (int* comm_old, int* ndims, int* dims, int* periods, int* reorder, int*  comm_cart, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Cart_create(get_comm(*comm_old), *ndims, dims, periods, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_cart = new_comm(tmp);
 }
}

void mpi_cart_get_ (int* comm, int* maxdims, int* dims, int* periods, int* coords, int* ierr) {
 *ierr = MPI_Cart_get(get_comm(*comm), *maxdims, dims, periods, coords);
}

void mpi_cart_map_ (int* comm_old, int* ndims, int* dims, int* periods, int* newrank, int* ierr) {
 *ierr = MPI_Cart_map(get_comm(*comm_old), *ndims, dims, periods, newrank);
}

void mpi_cart_rank_ (int* comm, int* coords, int* rank, int* ierr) {
 *ierr = MPI_Cart_rank(get_comm(*comm), coords, rank);
}

void mpi_cart_shift_ (int* comm, int* direction, int* displ, int* source, int* dest, int* ierr) {
 *ierr = MPI_Cart_shift(get_comm(*comm), *direction, *displ, source, dest);
}

void mpi_cart_sub_ (int* comm, int* remain_dims, int*  comm_new, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Cart_sub(get_comm(*comm), remain_dims, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_new = new_comm(tmp);
 }
}

void mpi_cartdim_get_ (int* comm, int* ndims, int* ierr) {
 *ierr = MPI_Cartdim_get(get_comm(*comm), ndims);
}

void mpi_graph_create_ (int* comm_old, int* nnodes, int* index, int* edges, int* reorder, int*  comm_graph, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Graph_create(get_comm(*comm_old), *nnodes, index, edges, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_graph = new_comm(tmp);
 }
}

void mpi_graph_get_ (int* comm, int* maxindex, int* maxedges, int* index, int* edges, int* ierr) {
 *ierr = MPI_Graph_get(get_comm(*comm), *maxindex, *maxedges, index, edges);
}

void mpi_graph_map_ (int* comm_old, int* nnodes, int* index, int* edges, int* newrank, int* ierr) {
 *ierr = MPI_Graph_map(get_comm(*comm_old), *nnodes, index, edges, newrank);
}

void mpi_graph_neighbors_ (int* comm, int* rank, int* maxneighbors, int* neighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors(get_comm(*comm), *rank, *maxneighbors, neighbors);
}

void mpi_graph_neighbors_count_ (int* comm, int* rank, int* nneighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors_count(get_comm(*comm), *rank, nneighbors);
}

void mpi_graphdims_get_ (int* comm, int* nnodes, int* nedges, int* ierr) {
 *ierr = MPI_Graphdims_get(get_comm(*comm), nnodes, nedges);
}

void mpi_topo_test_ (int* comm, int* top_type, int* ierr) {
 *ierr = MPI_Topo_test(get_comm(*comm), top_type);
}

void mpi_error_class_ (int* errorcode, int* errorclass, int* ierr) {
 *ierr = MPI_Error_class(*errorcode, errorclass);
}

void mpi_errhandler_create_ (void* function, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_create((MPI_Handler_function*)function, (MPI_Errhandler*)errhandler);
}

void mpi_errhandler_free_ (void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_free((MPI_Errhandler*)errhandler);
}

void mpi_errhandler_get_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_get(get_comm(*comm), (MPI_Errhandler*) errhandler);
}

void mpi_errhandler_set_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(get_comm(*comm), *(MPI_Errhandler*)errhandler);
}

void mpi_comm_set_errhandler_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(get_comm(*comm), *(MPI_Errhandler*)errhandler);
}

void mpi_comm_get_errhandler_ (int* comm, void* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(get_comm(*comm), (MPI_Errhandler*)errhandler);
}

void mpi_type_contiguous_ (int* count, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_contiguous(*count, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_cancel_ (int* request, int* ierr) {
  MPI_Request tmp=find_request(*request);
 *ierr = MPI_Cancel(&tmp);
 if(*ierr == MPI_SUCCESS) {
   free_request(*request);
 }
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
    reqs[i] = find_request(requests[i]);
    indices[i]=0;
  }
  *ierr = MPI_Testsome(*incount, reqs, outcount, indices, F2C_STATUSES_IGNORE(statuses));
  for(i=0;i<*incount;i++){
    if(indices[i]){
      if(reqs[indices[i]]==MPI_REQUEST_NULL){
          free_request(requests[indices[i]]);
          requests[indices[i]]=MPI_FORTRAN_REQUEST_NULL;
      }
    }
  }
  free(reqs);
}

void mpi_comm_test_inter_ (int* comm, int* flag, int* ierr) {
 *ierr = MPI_Comm_test_inter(get_comm(*comm), flag);
}

void mpi_unpack_ (void* inbuf, int* insize, int* position, void* outbuf, int* outcount, int* type, int* comm, int* ierr) {
 *ierr = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, get_datatype(*type), get_comm(*comm));
}

void mpi_pack_external_size_ (char *datarep, int* incount, int* datatype, MPI_Aint *size, int* ierr){
 *ierr = MPI_Pack_external_size(datarep, *incount, get_datatype(*datatype), size);
}

void mpi_pack_external_ (char *datarep, void *inbuf, int* incount, int* datatype, void *outbuf, MPI_Aint* outcount, MPI_Aint *position, int* ierr){
 *ierr = MPI_Pack_external(datarep, inbuf, *incount, get_datatype(*datatype), outbuf, *outcount, position);
}

void mpi_unpack_external_ ( char *datarep, void *inbuf, MPI_Aint* insize, MPI_Aint *position, void *outbuf, int* outcount, int* datatype, int* ierr){
 *ierr = MPI_Unpack_external( datarep, inbuf, *insize, position, outbuf, *outcount, get_datatype(*datatype));
}

void mpi_type_hindexed_ (int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_hindexed(*count, blocklens, indices, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_hindexed_ (int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed(*count, blocklens, indices, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_hindexed_block_ (int* count, int* blocklength, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed_block(*count, *blocklength, indices, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_indexed_ (int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_indexed(*count, blocklens, indices, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_indexed_block_ (int* count, int* blocklength, int* indices,  int* old_type,  int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_indexed_block(*count, *blocklength, indices, get_datatype(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_struct_ (int* count, int* blocklens, MPI_Aint* indices, int* old_types, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_struct(*count, blocklens, indices, (MPI_Datatype*)old_types, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_struct_ (int* count, int* blocklens, MPI_Aint* indices, int*  old_types, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_struct(*count, blocklens, indices, (MPI_Datatype*)old_types, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_ssend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr) {
 *ierr = MPI_Ssend(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm));
}

void mpi_ssend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr) {
  MPI_Request tmp;
 *ierr = MPI_Ssend_init(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_intercomm_create_ (int* local_comm, int *local_leader, int* peer_comm, int* remote_leader, int* tag, int*  comm_out, int* ierr) {
  MPI_Comm tmp;
  *ierr = MPI_Intercomm_create(get_comm(*local_comm), *local_leader,get_comm(*peer_comm), *remote_leader, *tag, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = new_comm(tmp);
  }
}

void mpi_intercomm_merge_ (int* comm, int* high, int*  comm_out, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Intercomm_merge(get_comm(*comm), *high, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_out = new_comm(tmp);
 }
}

void mpi_bsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int* ierr) {
 *ierr = MPI_Bsend(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm));
}

void mpi_bsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Bsend_init(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_ibsend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Ibsend(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_comm_remote_group_ (int* comm, int*  group, int* ierr) {
  MPI_Group tmp;
 *ierr = MPI_Comm_remote_group(get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *group = new_group(tmp);
 }
}

void mpi_comm_remote_size_ (int* comm, int* size, int* ierr) {
 *ierr = MPI_Comm_remote_size(get_comm(*comm), size);
}

void mpi_issend_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Issend(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_probe_ (int* source, int* tag, int* comm, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Probe(*source, *tag, get_comm(*comm), F2C_STATUS_IGNORE(status));
}

void mpi_attr_delete_ (int* comm, int* keyval, int* ierr) {
 *ierr = MPI_Attr_delete(get_comm(*comm), *keyval);
}

void mpi_attr_put_ (int* comm, int* keyval, void* attr_value, int* ierr) {
 *ierr = MPI_Attr_put(get_comm(*comm), *keyval, attr_value);
}

void mpi_rsend_init_ (void* buf, int* count, int* datatype, int *dest, int* tag, int* comm, int*  request, int* ierr) {
  MPI_Request tmp;
  *ierr = MPI_Rsend_init(buf, *count, get_datatype(*datatype), *dest, *tag, get_comm(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_keyval_create_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr) {
 *ierr = MPI_Keyval_create((MPI_Copy_function*)copy_fn, (MPI_Delete_function*)delete_fn, keyval, extra_state);
}

void mpi_keyval_free_ (int* keyval, int* ierr) {
 *ierr = MPI_Keyval_free(keyval);
}

void mpi_test_cancelled_ (MPI_Status*  status, int* flag, int* ierr) {
 *ierr = MPI_Test_cancelled(status, flag);
}

void mpi_pack_ (void* inbuf, int* incount, int* type, void* outbuf, int* outcount, int* position, int* comm, int* ierr) {
 *ierr = MPI_Pack(inbuf, *incount, get_datatype(*type), outbuf, *outcount, position, get_comm(*comm));
}

void mpi_get_elements_ (MPI_Status*  status, int* datatype, int* elements, int* ierr) {
 *ierr = MPI_Get_elements(status, get_datatype(*datatype), elements);
}

void mpi_dims_create_ (int* nnodes, int* ndims, int* dims, int* ierr) {
 *ierr = MPI_Dims_create(*nnodes, *ndims, dims);
}

void mpi_iprobe_ (int* source, int* tag, int* comm, int* flag, MPI_Status*  status, int* ierr) {
 *ierr = MPI_Iprobe(*source, *tag, get_comm(*comm), flag, status);
}

void mpi_type_get_envelope_ ( int* datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner, int* ierr){

 *ierr = MPI_Type_get_envelope(  get_datatype(*datatype), num_integers,
 num_addresses, num_datatypes, combiner);
}

void mpi_type_get_contents_ (int* datatype, int* max_integers, int* max_addresses, int* max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses,
 int* array_of_datatypes, int* ierr){
 *ierr = MPI_Type_get_contents(get_datatype(*datatype), *max_integers, *max_addresses,*max_datatypes, array_of_integers, array_of_addresses, (MPI_Datatype*)array_of_datatypes);
}

void mpi_type_create_darray_ (int* size, int* rank, int* ndims, int* array_of_gsizes, int* array_of_distribs, int* array_of_dargs, int* array_of_psizes,
 int* order, int* oldtype, int*newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_darray(*size, *rank, *ndims,  array_of_gsizes,
  array_of_distribs,  array_of_dargs,  array_of_psizes,
  *order,  get_datatype(*oldtype), &tmp) ;
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_resized_ (int* oldtype,MPI_Aint* lb, MPI_Aint* extent, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_resized(get_datatype(*oldtype),*lb, *extent, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_create_subarray_ (int* ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts, int* order, int* oldtype, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_subarray(*ndims,array_of_sizes, array_of_subsizes, array_of_starts, *order, get_datatype(*oldtype), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = new_datatype(tmp);
  }
}

void mpi_type_match_size_ (int* typeclass,int* size,int* datatype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_match_size(*typeclass,*size,&tmp);
  if(*ierr == MPI_SUCCESS) {
    *datatype = new_datatype(tmp);
  }
}

void mpi_alltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int* sendtypes, void *recvbuf, int *recvcnts, int *rdispls, int* recvtypes,
 int* comm, int* ierr){
 *ierr = MPI_Alltoallw( sendbuf, sendcnts, sdispls, (MPI_Datatype*) sendtypes, recvbuf, recvcnts, rdispls, (MPI_Datatype*)recvtypes, get_comm(*comm));
}

void mpi_exscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr){
 *ierr = MPI_Exscan(sendbuf, recvbuf, *count, get_datatype(*datatype), get_op(*op), get_comm(*comm));
}

void mpi_comm_set_name_ (int* comm, char* name, int* ierr){
 *ierr = MPI_Comm_set_name (get_comm(*comm), name);
}

void mpi_comm_dup_with_info_ (int* comm, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_dup_with_info(get_comm(*comm),*(MPI_Info*)info,&tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_comm_split_type_ (int* comm, int* split_type, int* key, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_split_type(get_comm(*comm), *split_type, *key, *(MPI_Info*)info, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_comm_set_info_ (int* comm, int* info, int* ierr){
 *ierr = MPI_Comm_set_info (get_comm(*comm), *(MPI_Info*)info);
}

void mpi_comm_get_info_ (int* comm, int* info, int* ierr){
 *ierr = MPI_Comm_get_info (get_comm(*comm), (MPI_Info*)info);
}

void mpi_info_get_ (int* info,char *key,int* valuelen, char *value, int *flag, int* ierr){
 *ierr = MPI_Info_get(*(MPI_Info*)info,key,*valuelen, value, flag);
}

void mpi_comm_create_errhandler_ ( void *function, void *errhandler, int* ierr){
 *ierr = MPI_Comm_create_errhandler( (MPI_Comm_errhandler_fn*) function, (MPI_Errhandler*)errhandler);
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
 *ierr = MPI_Comm_call_errhandler(get_comm(*comm), *errorcode);
}

void mpi_info_dup_ (int* info, int* newinfo, int* ierr){
 *ierr = MPI_Info_dup(*(MPI_Info*)info, (MPI_Info*)newinfo);
}

void mpi_info_get_valuelen_ ( int* info, char *key, int *valuelen, int *flag, int* ierr){
 *ierr = MPI_Info_get_valuelen( *(MPI_Info*)info, key, valuelen, flag);
}

void mpi_info_delete_ (int* info, char *key, int* ierr){
 *ierr = MPI_Info_delete(*(MPI_Info*)info, key);
}

void mpi_info_get_nkeys_ ( int* info, int *nkeys, int* ierr){
 *ierr = MPI_Info_get_nkeys(  *(MPI_Info*)info, nkeys);
}

void mpi_info_get_nthkey_ ( int* info, int* n, char *key, int* ierr){
 *ierr = MPI_Info_get_nthkey( *(MPI_Info*)info, *n, key);
}

void mpi_get_version_ (int *version,int *subversion, int* ierr){
 *ierr = MPI_Get_version (version,subversion);
}

void mpi_get_library_version_ (char *version,int *len, int* ierr){
 *ierr = MPI_Get_library_version (version,len);
}

void mpi_request_get_status_ ( int* request, int *flag, MPI_Status* status, int* ierr){
 *ierr = MPI_Request_get_status( find_request(*request), flag, status);
}

void mpi_grequest_start_ ( void *query_fn, void *free_fn, void *cancel_fn, void *extra_state, int*request, int* ierr){
  MPI_Request tmp;
  *ierr = MPI_Grequest_start( (MPI_Grequest_query_function*)query_fn, (MPI_Grequest_free_function*)free_fn, (MPI_Grequest_cancel_function*)cancel_fn, extra_state, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = new_request(tmp);
 }
}

void mpi_grequest_complete_ ( int* request, int* ierr){
 *ierr = MPI_Grequest_complete( find_request(*request));
}

void mpi_status_set_cancelled_ (MPI_Status* status,int* flag, int* ierr){
 *ierr = MPI_Status_set_cancelled(status,*flag);
}

void mpi_status_set_elements_ ( MPI_Status* status, int* datatype, int* count, int* ierr){
 *ierr = MPI_Status_set_elements( status, get_datatype(*datatype), *count);
}

void mpi_comm_connect_ ( char *port_name, int* info, int* root, int* comm, int*newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_connect( port_name, *(MPI_Info*)info, *root, get_comm(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_publish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Publish_name( service_name, *(MPI_Info*)info, port_name);
}

void mpi_unpublish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Unpublish_name( service_name, *(MPI_Info*)info, port_name);
}

void mpi_lookup_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Lookup_name( service_name, *(MPI_Info*)info, port_name);
}

void mpi_comm_join_ ( int* fd, int* intercomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_join( *fd, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = new_comm(tmp);
  }
}

void mpi_open_port_ ( int* info, char *port_name, int* ierr){
 *ierr = MPI_Open_port( *(MPI_Info*)info,port_name);
}

void mpi_close_port_ ( char *port_name, int* ierr){
 *ierr = MPI_Close_port( port_name);
}

void mpi_comm_accept_ ( char *port_name, int* info, int* root, int* comm, int*newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_accept( port_name, *(MPI_Info*)info, *root, get_comm(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_comm_spawn_ ( char *command, char *argv, int* maxprocs, int* info, int* root, int* comm, int* intercomm, int* array_of_errcodes, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_spawn( command, NULL, *maxprocs, *(MPI_Info*)info, *root, get_comm(*comm), &tmp, array_of_errcodes);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = new_comm(tmp);
  }
}

void mpi_comm_spawn_multiple_ ( int* count, char *array_of_commands, char** array_of_argv, int* array_of_maxprocs, int* array_of_info, int* root,
 int* comm, int* intercomm, int* array_of_errcodes, int* ierr){
 MPI_Comm tmp;
 *ierr = MPI_Comm_spawn_multiple(* count, &array_of_commands, &array_of_argv, array_of_maxprocs,
 (MPI_Info*)array_of_info, *root, get_comm(*comm), &tmp, array_of_errcodes);
 if(*ierr == MPI_SUCCESS) {
   *intercomm = new_comm(tmp);
 }
}

void mpi_comm_get_parent_ ( int* parent, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_get_parent( &tmp);
  if(*ierr == MPI_SUCCESS) {
    *parent = new_comm(tmp);
  }
}
