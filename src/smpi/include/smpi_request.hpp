/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_REQUEST_HPP_INCLUDED
#define SMPI_REQUEST_HPP_INCLUDED

#include "smpi/smpi.h"
#include "smpi_f2c.hpp"

#include <memory>

namespace simgrid{
namespace smpi{

struct smpi_mpi_generalized_request_funcs_t {
  MPI_Grequest_query_function *query_fn;
  MPI_Grequest_free_function *free_fn;
  MPI_Grequest_cancel_function *cancel_fn;
  void* extra_state;
  s4u::ConditionVariablePtr cond;
  s4u::MutexPtr mutex;
};

class Request : public F2C {
  void* buf_;
  /* in the case of non-contiguous memory the user address should be keep
   * to unserialize the data inside the user memory*/
  void* old_buf_;
  /* this is especially for derived datatypes that we need to serialize/unserialize.
   * It let us know how to unserialize at the end of the communication */
  MPI_Datatype type_;
  size_t size_;
  aid_t src_;
  aid_t dst_;
  int tag_;
  // to handle cases where we have an unknown sender
  // We can't override src, tag, and size, because the request may be reused later
  aid_t real_src_;
  int real_tag_;
  bool truncated_;
  bool unmatched_types_;
  size_t real_size_;
  MPI_Comm comm_;
  simgrid::kernel::activity::ActivityImplPtr action_;
  unsigned flags_;
  bool detached_;
  MPI_Request detached_sender_;
  int refcount_;
  unsigned int message_id_;
  MPI_Op op_;
  std::unique_ptr<smpi_mpi_generalized_request_funcs_t> generalized_funcs;
  std::vector<MPI_Request> nbc_requests_;
  s4u::Host* src_host_ = nullptr; //!< save simgrid's source host since it can finished before the recv
  static bool match_common(MPI_Request req, MPI_Request sender, MPI_Request receiver);
  static bool match_types(MPI_Datatype stype, MPI_Datatype rtype);

public:
  Request() = default;
  Request(const void* buf, int count, MPI_Datatype datatype, aid_t src, aid_t dst, int tag, MPI_Comm comm,
          unsigned flags, MPI_Op op = MPI_REPLACE);
  MPI_Comm comm() const { return comm_; }
  size_t size() const { return size_; }
  size_t real_size() const { return real_size_; }
  aid_t src() const { return src_; }
  aid_t dst() const { return dst_; }
  int tag() const { return tag_; }
  int flags() const { return flags_; }
  bool detached() const { return detached_; }
  std::string name() const override { return std::string("MPI_Request"); }
  MPI_Datatype type() const { return type_; }
  void print_request(const char* message) const;
  void start();
  void cancel();
  void init_buffer(int count);
  void ref();
  void start_nbc_requests(std::vector<MPI_Request> reqs);
  static int finish_nbc_requests(MPI_Request* req, int test);
  std::vector<MPI_Request> get_nbc_requests() const;
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

  static int recv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status);
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
