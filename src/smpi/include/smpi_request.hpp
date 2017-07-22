/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_REQUEST_HPP_INCLUDED
#define SMPI_REQUEST_HPP_INCLUDED

#include "smpi/smpi.h"
#include "smpi_f2c.hpp"

namespace simgrid{
namespace smpi{

class Request : public F2C {
  private :
    void *buf_;
    /* in the case of non-contiguous memory the user address should be keep
     * to unserialize the data inside the user memory*/
    void *old_buf_;
    /* this let us know how to unserialize at the end of
     * the communication*/
    MPI_Datatype old_type_;
    size_t size_;
    int src_;
    int dst_;
    int tag_;
    //to handle cases where we have an unknown sender
    //We can't override src, tag, and size, because the request may be reused later
    int real_src_;
    int real_tag_;
    int truncated_;
    size_t real_size_;
    MPI_Comm comm_;
    smx_activity_t action_;
    unsigned flags_;
    int detached_;
    MPI_Request detached_sender_;
    int refcount_;
    MPI_Op op_;
  public:
    Request()=default;
    Request(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm, unsigned flags);
    MPI_Comm comm();
    MPI_Datatype old_type();
    size_t size();
    size_t real_size();
    int src();
    int dst();
    int tag();
    int flags();
    int detached();
    void print_request(const char *message);
    void start();

    static void finish_wait(MPI_Request* request, MPI_Status * status);
    static void unref(MPI_Request* request);
    static void wait(MPI_Request* req, MPI_Status * status);
    static MPI_Request send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static MPI_Request isend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static MPI_Request ssend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static MPI_Request rma_send_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,MPI_Op op);
    static MPI_Request recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
    static MPI_Request rma_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,MPI_Op op);
    static MPI_Request irecv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
    static MPI_Request isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static MPI_Request issend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static MPI_Request irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);

    static void recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status);
    static void send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
    static void ssend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);

    static void sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status);

    static void startall(int count, MPI_Request * requests);

    static int test(MPI_Request * request,MPI_Status * status);
    static int testsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[]);
    static int testany(int count, MPI_Request requests[], int *index, MPI_Status * status);
    static int testall(int count, MPI_Request requests[], MPI_Status status[]);

    static void probe(int source, int tag, MPI_Comm comm, MPI_Status* status);
    static void iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status);

    static int waitany(int count, MPI_Request requests[], MPI_Status * status);
    static int waitall(int count, MPI_Request requests[], MPI_Status status[]);
    static int waitsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[]);

    static int match_send(void* a, void* b, simgrid::kernel::activity::CommImpl* ignored);
    static int match_recv(void* a, void* b, simgrid::kernel::activity::CommImpl* ignored);

    int add_f();
    static void free_f(int id);
    static Request* f2c(int);

};


}
}

#endif
