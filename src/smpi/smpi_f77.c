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
static xbt_dict_t request_lookup = NULL;
static xbt_dynar_t datatype_lookup = NULL;
static xbt_dynar_t op_lookup = NULL;

#define KEY_SIZE (sizeof(int) * 2 + 1)

static int new_comm(MPI_Comm comm) {
  xbt_dynar_push(comm_lookup, &comm);
  return (int)xbt_dynar_length(comm_lookup) - 1;
}

static MPI_Comm get_comm(int comm) {
  if(comm == -2) {
    return MPI_COMM_SELF;
  } else if(comm >= 0) {
    return *(MPI_Comm*)xbt_dynar_get_ptr(comm_lookup, comm);
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

static int new_op(MPI_Op op) {
  xbt_dynar_push(op_lookup, &op);
  return (int)xbt_dynar_length(op_lookup) - 1;
}

static MPI_Op get_op(int op) {
   return op >= 0
          ? *(MPI_Op*)xbt_dynar_get_ptr(op_lookup, op)
          : MPI_OP_NULL;
}

void mpi_init__(int* ierr) {
   comm_lookup = xbt_dynar_new(sizeof(MPI_Comm), NULL);
   new_comm(MPI_COMM_WORLD);

   request_lookup = xbt_dict_new();

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

void mpi_finalize__(int* ierr) {
   *ierr = MPI_Finalize();
   xbt_dynar_free(&op_lookup);
   xbt_dynar_free(&datatype_lookup);
   xbt_dict_free(&request_lookup);
   xbt_dynar_free(&comm_lookup);
}

void mpi_abort__(int* comm, int* errorcode, int* ierr) {
  *ierr = MPI_Abort(get_comm(*comm), *errorcode);
}

void mpi_comm_rank__(int* comm, int* rank, int* ierr) {
   *ierr = MPI_Comm_rank(get_comm(*comm), rank);
}

void mpi_comm_size__(int* comm, int* size, int* ierr) {
   *ierr = MPI_Comm_size(get_comm(*comm), size);
}

double mpi_wtime__(void) {
   return MPI_Wtime();
}

void mpi_comm_dup__(int* comm, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_dup(get_comm(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = new_comm(tmp);
  }
}

void mpi_comm_split__(int* comm, int* color, int* key, int* comm_out, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_split(get_comm(*comm), *color, *key, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = new_comm(tmp);
  }
}

void mpi_send_init__(void *buf, int* count, int* datatype, int* dst, int* tag,
                     int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Send_init(buf, *count, get_datatype(*datatype), *dst, *tag,
                        get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_isend__(void *buf, int* count, int* datatype, int* dst,
                 int* tag, int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Isend(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_send__(void* buf, int* count, int* datatype, int* dst,
                int* tag, int* comm, int* ierr) {
   *ierr = MPI_Send(buf, *count, get_datatype(*datatype), *dst, *tag,
                    get_comm(*comm));
}

void mpi_recv_init__(void *buf, int* count, int* datatype, int* src, int* tag,
                     int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Recv_init(buf, *count, get_datatype(*datatype), *src, *tag,
                        get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_irecv__(void *buf, int* count, int* datatype, int* src, int* tag,
                 int* comm, int* request, int* ierr) {
  MPI_Request req;

  *ierr = MPI_Irecv(buf, *count, get_datatype(*datatype), *src, *tag,
                    get_comm(*comm), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = new_request(req);
  }
}

void mpi_recv__(void* buf, int* count, int* datatype, int* src,
                int* tag, int* comm, MPI_Status* status, int* ierr) {
   *ierr = MPI_Recv(buf, *count, get_datatype(*datatype), *src, *tag,
                    get_comm(*comm), status);
}

void mpi_start__(int* request, int* ierr) {
  MPI_Request req = find_request(*request);

  *ierr = MPI_Start(&req);
}

void mpi_startall__(int* count, int* requests, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Startall(*count, reqs);
  free(reqs);
}

void mpi_wait__(int* request, MPI_Status* status, int* ierr) {
   MPI_Request req = find_request(*request);
   
   *ierr = MPI_Wait(&req, status);
}

void mpi_waitany__(int* count, int* requests, int* index, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitany(*count, reqs, index, status);
  free(reqs);
}

void mpi_waitall__(int* count, int* requests, MPI_Status* status, int* ierr) {
  MPI_Request* reqs;
  int i;

  reqs = xbt_new(MPI_Request, *count);
  for(i = 0; i < *count; i++) {
    reqs[i] = find_request(requests[i]);
  }
  *ierr = MPI_Waitall(*count, reqs, status);
  free(reqs);
}

void mpi_barrier__(int* comm, int* ierr) {
  *ierr = MPI_Barrier(get_comm(*comm));
}

void mpi_bcast__(void *buf, int* count, int* datatype, int* root, int* comm, int* ierr) {
  *ierr = MPI_Bcast(buf, *count, get_datatype(*datatype), *root, get_comm(*comm));
}

void mpi_reduce__(void* sendbuf, void* recvbuf, int* count,
                  int* datatype, int* op, int* root, int* comm, int* ierr) {
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count,
                     get_datatype(*datatype), get_op(*op), *root, get_comm(*comm));
}

void mpi_allreduce__(void* sendbuf, void* recvbuf, int* count, int* datatype,
                     int* op, int* comm, int* ierr) {
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, get_datatype(*datatype),
                        get_op(*op), get_comm(*comm));
}

void mpi_scatter__(void* sendbuf, int* sendcount, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, 
                   int* root, int* comm, int* ierr) {
  *ierr = MPI_Scatter(sendbuf, *sendcount, get_datatype(*sendtype),
                      recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_gather__(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr) {
  *ierr = MPI_Gather(sendbuf, *sendcount, get_datatype(*sendtype),
                     recvbuf, *recvcount, get_datatype(*recvtype), *root, get_comm(*comm));
}

void mpi_allgather__(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr) {
  *ierr = MPI_Allgather(sendbuf, *sendcount, get_datatype(*sendtype),
                        recvbuf, *recvcount, get_datatype(*recvtype), get_comm(*comm));
}

void mpi_scan__(void* sendbuf, void* recvbuf, int* count, int* datatype,
                int* op, int* comm, int* ierr) {
  *ierr = MPI_Scan(sendbuf, recvbuf, *count, get_datatype(*datatype),
                   get_op(*op), get_comm(*comm));
}

void mpi_alltoall__(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr) {
  *ierr = MPI_Alltoall(sendbuf, *sendcount, get_datatype(*sendtype),
                       recvbuf, *recvcount, get_datatype(*recvtype), get_comm(*comm));
}
