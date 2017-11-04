/* smpi_coll.c -- various optimized routing for collectives                 */

/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_coll.hpp"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"
#include "smpi_request.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi, "Logging specific to SMPI (coll)");

#define COLL_SETTER(cat, ret, args, args2)                                                                             \
  int(*Colls::cat) args;                                                                                               \
  void Colls::set_##cat(std::string name)                                                                              \
  {                                                                                                                    \
    int id = find_coll_description(mpi_coll_##cat##_description, name, #cat);                                          \
    cat    = reinterpret_cast<ret(*) args>(mpi_coll_##cat##_description[id].coll);                                     \
    if (cat == nullptr)                                                                                                \
      xbt_die("Collective " #cat " set to nullptr!");                                                                  \
  }

#define SET_COLL(coll)                                                                                                 \
  name = xbt_cfg_get_string("smpi/" #coll);                                                                            \
  if (name.empty())                                                                                                    \
    name = selector_name;                                                                                              \
  set_##coll(name);

namespace simgrid{
namespace smpi{

void (*Colls::smpi_coll_cleanup_callback)();

/* these arrays must be nullptr terminated */
s_mpi_coll_description_t Colls::mpi_coll_gather_description[] = {
    COLL_GATHERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_allgather_description[] = {
    COLL_ALLGATHERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_allgatherv_description[] = {
    COLL_ALLGATHERVS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_allreduce_description[] ={
    COLL_ALLREDUCES(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_reduce_scatter_description[] = {
    COLL_REDUCE_SCATTERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_scatter_description[] ={
    COLL_SCATTERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_barrier_description[] ={
    COLL_BARRIERS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_alltoall_description[] = {
    COLL_ALLTOALLS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_alltoallv_description[] = {
    COLL_ALLTOALLVS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_bcast_description[] = {
    COLL_BCASTS(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };
s_mpi_coll_description_t Colls::mpi_coll_reduce_description[] = {
    COLL_REDUCES(COLL_DESCRIPTION, COLL_COMMA), {nullptr, nullptr, nullptr} };

/** Displays the long description of all registered models, and quit */
void Colls::coll_help(const char *category, s_mpi_coll_description_t * table)
{
  XBT_WARN("Long description of the %s models accepted by this simulator:\n", category);
  for (int i = 0; table[i].name; i++)
    XBT_WARN("  %s: %s\n", table[i].name, table[i].description);
}

int Colls::find_coll_description(s_mpi_coll_description_t* table, std::string name, const char* desc)
{
  for (int i = 0; table[i].name; i++)
    if (name == table[i].name) {
      if (strcmp(table[i].name,"default"))
        XBT_INFO("Switch to algorithm %s for collective %s",table[i].name,desc);
      return i;
    }

  if (not table[0].name)
    xbt_die("No collective is valid for '%s'! This is a bug.", name.c_str());
  std::string name_list = std::string(table[0].name);
  for (int i = 1; table[i].name; i++)
    name_list = name_list + ", " + table[i].name;

  xbt_die("Collective '%s' is invalid! Valid collectives are: %s.", name.c_str(), name_list.c_str());
  return -1;
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
  std::string selector_name = xbt_cfg_get_string("smpi/coll-selector");
  if (selector_name.empty())
    selector_name = "default";

  std::string name;

  SET_COLL(gather);
  SET_COLL(allgather);
  SET_COLL(allgatherv);
  SET_COLL(allreduce);
  SET_COLL(alltoall);
  SET_COLL(alltoallv);
  SET_COLL(reduce);
  SET_COLL(reduce_scatter);
  SET_COLL(scatter);
  SET_COLL(bcast);
  SET_COLL(barrier);
}


//Implementations of the single algorith collectives

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

  // Send/Recv buffers to/from others
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
        op->apply( tmpbufs[index], recvbuf, &count, datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
      Request::wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank && op!=MPI_OP_NULL) {
        op->apply( tmpbufs[other], recvbuf, &count, datatype);
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

  // Send/Recv buffers to/from others
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
          op->apply( tmpbufs[index], recvbuf, &count, datatype);
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
          if(op!=MPI_OP_NULL)
            op->apply( tmpbufs[other], recvbuf, &count, datatype);
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
