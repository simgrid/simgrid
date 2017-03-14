/*High level handling of collective algorithms*/
/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COLL_HPP
#define SMPI_COLL_HPP

#include <xbt/base.h>

#include "private.h"

namespace simgrid{
namespace smpi{

class Colls{
  private:
  public:
    static void set_collectives();
    static void set_gather(const char* name);
    static void set_allgather(const char* name);
    static void set_allgatherv(const char* name);
    static void set_alltoall(const char* name);
    static void set_alltoallv(const char* name);
    static void set_allreduce(const char* name);
    static void set_reduce(const char* name);
    static void set_reduce_scatter(const char* name);
    static void set_scatter(const char* name);
    static void set_barrier(const char* name);
    static void set_bcast(const char* name);

    static void coll_help(const char *category, s_mpi_coll_description_t * table);

    static int (*gather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
    static int (*allgather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    static int (*allgatherv)(void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
    static int (*allreduce)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
    static int (*alltoall)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    static int (*alltoallv)(void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
    static int (*bcast)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
    static int (*reduce)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
    static int (*reduce_scatter)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
    static int (*scatter)(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
    static int (*barrier)(MPI_Comm comm);

//These fairly unused collectives only have one implementation in SMPI

    static int gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
    static int scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
    static int scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    static int exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
};

class Coll_algo{
  private:
    char* description_;
  public:
    char* description();
};

class Coll_gather : public Coll_algo {
  private:
  public:
    static int gather (void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
};

class Coll_allgather : public Coll_algo {
  private:
  public:
    static int allgather (void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
};

class Coll_allgatherv : public Coll_algo {
  private:
  public:
    static int allgatherv (void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
};

class Coll_allreduce : public Coll_algo {
  private:
  public:
    static int allreduce (void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
};

class Coll_alltoall : public Coll_algo {
  private:
  public:
    static int alltoall (void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
};

class Coll_alltoallv : public Coll_algo {
  private:
  public:
    static int alltoallv (void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
};

class Coll_bcast : public Coll_algo {
  private:
  public:
    static int bcast (void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
};

class Coll_reduce : public Coll_algo {
  private:
  public:
    static int reduce (void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
};

class Coll_reduce_scatter : public Coll_algo {
  private:
  public:
    static int reduce_scatter (void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
};

class Coll_scatter : public Coll_algo {
  private:
  public:
    static int scatter (void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
};

class Coll_barrier : public Coll_algo {
  private:
  public:
    static int barrier (MPI_Comm);
};


}
}

#endif
