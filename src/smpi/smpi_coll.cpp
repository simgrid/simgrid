/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "colls/colls.h"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi, "Logging specific to SMPI (coll)");

s_mpi_coll_description_t mpi_coll_gather_description[] = {
   COLL_GATHERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}  /* this array must be nullptr terminated */
};

s_mpi_coll_description_t mpi_coll_allgather_description[] = {
   COLL_ALLGATHERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}
};

s_mpi_coll_description_t mpi_coll_allgatherv_description[] = { COLL_ALLGATHERVS(COLL_DESCRIPTION, COLL_COMMA),
   {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_mpi_coll_description_t mpi_coll_allreduce_description[] ={ COLL_ALLREDUCES(COLL_DESCRIPTION, COLL_COMMA),
  {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_mpi_coll_description_t mpi_coll_reduce_scatter_description[] = {COLL_REDUCE_SCATTERS(COLL_DESCRIPTION, COLL_COMMA),
    {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_mpi_coll_description_t mpi_coll_scatter_description[] ={COLL_SCATTERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}};

s_mpi_coll_description_t mpi_coll_barrier_description[] ={COLL_BARRIERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}};

s_mpi_coll_description_t mpi_coll_alltoall_description[] = {COLL_ALLTOALLS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}};

s_mpi_coll_description_t mpi_coll_alltoallv_description[] = {COLL_ALLTOALLVS(COLL_DESCRIPTION, COLL_COMMA),
   {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_mpi_coll_description_t mpi_coll_bcast_description[] = {COLL_BCASTS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr}};

s_mpi_coll_description_t mpi_coll_reduce_description[] = {COLL_REDUCES(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };



/** Displays the long description of all registered models, and quit */
void coll_help(const char *category, s_mpi_coll_description_t * table)
{
  printf("Long description of the %s models accepted by this simulator:\n", category);
  for (int i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_coll_description(s_mpi_coll_description_t * table, const char *name, const char *desc)
{
  char *name_list = nullptr;
  int selector_on=0;
  if (name==nullptr || name[0] == '\0') {
    //no argument provided, use active selector's algorithm
    name=static_cast<char*>(xbt_cfg_get_string("smpi/coll-selector"));
    selector_on=1;
  }
  for (int i = 0; table[i].name; i++)
    if (!strcmp(name, table[i].name)) {
      if (strcmp(table[i].name,"default"))
        XBT_INFO("Switch to algorithm %s for collective %s",table[i].name,desc);
      return i;
    }

  if(selector_on){
    // collective seems not handled by the active selector, try with default one
    for (int i = 0; table[i].name; i++)
      if (!strcmp("default", table[i].name)) {
        return i;
    }
  }
  if (!table[0].name)
    xbt_die("No collective is valid for '%s'! This is a bug.",name);
  name_list = xbt_strdup(table[0].name);
  for (int i = 1; table[i].name; i++) {
    name_list = static_cast<char*>(xbt_realloc(name_list, strlen(name_list) + strlen(table[i].name) + 3));
    strncat(name_list, ", ",2);
    strncat(name_list, table[i].name, strlen(table[i].name));
  }
  xbt_die("Collective '%s' is invalid! Valid collectives are: %s.", name, name_list);
  return -1;
}

void (*smpi_coll_cleanup_callback)();

namespace simgrid{
namespace smpi{

int (*Colls::gather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
int (*Colls::allgather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*Colls::allgatherv)(void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
int (*Colls::allreduce)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int (*Colls::alltoall)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*Colls::alltoallv)(void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
int (*Colls::bcast)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
int (*Colls::reduce)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int (*Colls::reduce_scatter)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int (*Colls::scatter)(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
int (*Colls::barrier)(MPI_Comm comm);


#define COLL_SETTER(cat, ret, args, args2)\
void Colls::set_##cat (const char * name){\
    int id = find_coll_description(mpi_coll_## cat ##_description,\
                                             name,#cat);\
    cat = reinterpret_cast<ret (*) args>\
        (mpi_coll_## cat ##_description[id].coll);\
}

COLL_APPLY(COLL_SETTER,COLL_GATHER_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_ALLGATHER_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_ALLGATHERV_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_REDUCE_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_ALLREDUCE_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_REDUCE_SCATTER_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_SCATTER_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_BARRIER_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_BCAST_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_ALLTOALL_SIG,"");
COLL_APPLY(COLL_SETTER,COLL_ALLTOALLV_SIG,"");


void Colls::set_collectives(){
    const char* selector_name = static_cast<char*>(xbt_cfg_get_string("smpi/coll-selector"));
    if (selector_name==nullptr || selector_name[0] == '\0')
        selector_name = "default";

    const char* name = xbt_cfg_get_string("smpi/gather");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;
      
    set_gather(name);

    name = xbt_cfg_get_string("smpi/allgather");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_allgather(name);

    name = xbt_cfg_get_string("smpi/allgatherv");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_allgatherv(name);

    name = xbt_cfg_get_string("smpi/allreduce");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_allreduce(name);

    name = xbt_cfg_get_string("smpi/alltoall");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_alltoall(name);

    name = xbt_cfg_get_string("smpi/alltoallv");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_alltoallv(name);

    name = xbt_cfg_get_string("smpi/reduce");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_reduce(name);

    name = xbt_cfg_get_string("smpi/reduce-scatter");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_reduce_scatter(name);

    name = xbt_cfg_get_string("smpi/scatter");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_scatter(name);

    name = xbt_cfg_get_string("smpi/bcast");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_bcast(name);

    name = xbt_cfg_get_string("smpi/barrier");
    if (name==nullptr || name[0] == '\0')
        name = selector_name;

    set_barrier(name);
}


int Colls::gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_GATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if (rank != root) {
    // Send buffer to root
    Request::send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    recvtype->extent(&lb, &recvext);
    // Local copy from root
    Datatype::copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + displs[root] * recvext,
                       recvcounts[root], recvtype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = Request::irecv_init(static_cast<char*>(recvbuf) + displs[src] * recvext,
                          recvcounts[src], recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int src = 0; src < size-1; src++) {
      Request::unref(&requests[src]);
    }
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}


int Colls::scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_SCATTERV;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Recv buffer from root
    Request::recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    sendtype->extent(&lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
      Datatype::copy(static_cast<char *>(sendbuf) + displs[root] * sendext, sendcounts[root],
                       sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int dst = 0; dst < size; dst++) {
      if (dst != root) {
        requests[index] = Request::isend_init(static_cast<char *>(sendbuf) + displs[dst] * sendext, sendcounts[dst],
                            sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    Request::startall(size - 1, requests);
    Request::waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int dst = 0; dst < size-1; dst++) {
      Request::unref(&requests[dst]);
    }
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}


int Colls::scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb      = 0;
  MPI_Aint dataext = 0;

  int rank = comm->rank();
  int size = comm->size();

  datatype->extent(&lb, &dataext);

  // Local copy from self
  Datatype::copy(sendbuf, count, datatype, recvbuf, count, datatype);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = Request::irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = Request::isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  Request::startall(size - 1, requests);

  if(op != MPI_OP_NULL && op->is_commutative()){
    for (int other = 0; other < size - 1; other++) {
      index = Request::waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        // #Request is below rank: it's a irecv
        if(op!=MPI_OP_NULL) op->apply( tmpbufs[index], recvbuf, &count, datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
      Request::wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        if(op!=MPI_OP_NULL) op->apply( tmpbufs[other], recvbuf, &count, datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    Request::unref(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
  return MPI_SUCCESS;
}

int Colls::exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb         = 0;
  MPI_Aint dataext    = 0;
  int recvbuf_is_empty=1;
  int rank = comm->rank();
  int size = comm->size();

  datatype->extent(&lb, &dataext);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = Request::irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = Request::isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  Request::startall(size - 1, requests);

  if(op != MPI_OP_NULL && op->is_commutative()){
    for (int other = 0; other < size - 1; other++) {
      index = Request::waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        if(recvbuf_is_empty){
          Datatype::copy(tmpbufs[index], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty=0;
        } else
          // #Request is below rank: it's a irecv
          if(op!=MPI_OP_NULL) op->apply( tmpbufs[index], recvbuf, &count, datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
     Request::wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        if (recvbuf_is_empty) {
          Datatype::copy(tmpbufs[other], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty = 0;
        } else
          if(op!=MPI_OP_NULL) op->apply( tmpbufs[other], recvbuf, &count, datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    Request::unref(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
  return MPI_SUCCESS;
}

}
}





