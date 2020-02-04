/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_REQUEST_HPP_INCLUDED
#define SMPI_REQUEST_HPP_INCLUDED

#include "smpi/smpi.h"
#include "smpi_f2c.hpp"

namespace simgrid{
namespace smpi{

typedef struct s_smpi_mpi_generalized_request_funcs {
  MPI_Grequest_query_function *query_fn;
  MPI_Grequest_free_function *free_fn;
  MPI_Grequest_cancel_function *cancel_fn;
  void* extra_state;
  s4u::ConditionVariablePtr cond;
  s4u::MutexPtr mutex;
} s_smpi_mpi_generalized_request_funcs_t; 
typedef struct s_smpi_mpi_generalized_request_funcs *smpi_mpi_generalized_request_funcs;

class Request : public F2C {
  void* buf_;
  /* in the case of non-contiguous memory the user address should be keep
   * to unserialize the data inside the user memory*/
  void* old_buf_;
  /* this is especially for derived datatypes that we need to serialize/unserialize.
   * It let us know how to unserialize at the end of the communication */
  MPI_Datatype old_type_;
  size_t size_;
  int src_;
  int dst_;
  int tag_;
  // to handle cases where we have an unknown sender
  // We can't override src, tag, and size, because the request may be reused later
  int real_src_;
  int real_tag_;
  bool truncated_;
  size_t real_size_;
  MPI_Comm comm_;
  simgrid::kernel::activity::ActivityImplPtr action_;
  unsigned flags_;
  bool detached_;
  MPI_Request detached_sender_;
  int refcount_;
  MPI_Op op_;
  int cancelled_; // tri-state
  smpi_mpi_generalized_request_funcs generalized_funcs;
  MPI_Request* nbc_requests_;
  int nbc_requests_size_;
  static bool match_common(MPI_Request req, MPI_Request sender, MPI_Request receiver);

public:
  Request() = default;
  Request(const void* buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm, unsigned flags, MPI_Op op = MPI_REPLACE);
  MPI_Comm comm() { return comm_; }
  size_t size() { return size_; }
  size_t real_size() { return real_size_; }
  int src() { return src_; }
  int dst() { return dst_; }
  int tag() { return tag_; }
  int flags() { return flags_; }
  bool detached() { return detached_; }
  MPI_Datatype type() { return old_type_; }
  void print_request(const char* message);
  void start();
  void cancel();
  void ref();
  void set_nbc_requests(MPI_Request* reqs, int size);
  int get_nbc_requests_size();
  MPI_Request* get_nbc_requests();
  static void finish_wait(MPI_Request* request, MPI_Status* status);
  static void unref(MPI_Request* request);
  static int wait(MPI_Request* req, MPI_Status* status);
  static MPI_Request bsend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request send_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request isend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request ssend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request rma_send_init(const void* buf, int count, MPI_Datatype datatype, int src, int dst, int tag,
                                   MPI_Comm comm, MPI_Op op);
  static MPI_Request recv_init(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
  static MPI_Request rma_recv_init(void* buf, int count, MPI_Datatype datatype, int src, int dst, int tag,
                                   MPI_Comm comm, MPI_Op op);
  static MPI_Request irecv_init(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
  static MPI_Request ibsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request isend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request issend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static MPI_Request irecv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);

  static void recv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status);
  static void bsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static void send(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
  static void ssend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);

  static void sendrecv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf,
                       int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status);

  static void startall(int count, MPI_Request* requests);

  static int test(MPI_Request* request, MPI_Status* status, int* flag);
  static int testsome(int incount, MPI_Request requests[], int* outcounts, int* indices, MPI_Status status[]);
  static int testany(int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status);
  static int testall(int count, MPI_Request requests[], int* flag, MPI_Status status[]);

  static void probe(int source, int tag, MPI_Comm comm, MPI_Status* status);
  static void iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status);

  static int waitany(int count, MPI_Request requests[], MPI_Status* status);
  static int waitall(int count, MPI_Request requests[], MPI_Status status[]);
  static int waitsome(int incount, MPI_Request requests[], int* indices, MPI_Status status[]);

  static bool match_send(void* a, void* b, kernel::activity::CommImpl* ignored);
  static bool match_recv(void* a, void* b, kernel::activity::CommImpl* ignored);

  static int grequest_start( MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request);
  static int grequest_complete( MPI_Request request);
  static int get_status(const Request* req, int* flag, MPI_Status* status);

  static void free_f(int id);
  static Request* f2c(int);
};


}
}

#endif
