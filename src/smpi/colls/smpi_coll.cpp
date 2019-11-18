/* smpi_coll.c -- various optimized routing for collectives                 */

/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_coll.hpp"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"
#include "smpi_request.hpp"
#include "xbt/config.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi, "Logging specific to SMPI collectives.");

#define COLL_SETTER(cat, ret, args, args2)                                                                             \
  void colls::_XBT_CONCAT(set_, cat)(const std::string& name)                                                          \
  {                                                                                                                    \
    int id = find_coll_description(_XBT_CONCAT3(mpi_coll_, cat, _description), name, _XBT_STRINGIFY(cat));             \
    cat    = reinterpret_cast<ret(*) args>(_XBT_CONCAT3(mpi_coll_, cat, _description)[id].coll);                       \
    if (cat == nullptr)                                                                                                \
      xbt_die("Collective " _XBT_STRINGIFY(cat) " set to nullptr!");                                                   \
  }

namespace simgrid{
namespace smpi{

/* these arrays must be nullptr terminated */
s_mpi_coll_description_t mpi_coll_gather_description[]         = {COLL_GATHERS(COLL_DESCRIPTION, COLL_COMMA),
                                                          {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_allgather_description[]      = {COLL_ALLGATHERS(COLL_DESCRIPTION, COLL_COMMA),
                                                             {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_allgatherv_description[]     = {COLL_ALLGATHERVS(COLL_DESCRIPTION, COLL_COMMA),
                                                              {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_allreduce_description[]      = {COLL_ALLREDUCES(COLL_DESCRIPTION, COLL_COMMA),
                                                             {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_reduce_scatter_description[] = {COLL_REDUCE_SCATTERS(COLL_DESCRIPTION, COLL_COMMA),
                                                                  {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_scatter_description[]        = {COLL_SCATTERS(COLL_DESCRIPTION, COLL_COMMA),
                                                           {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_barrier_description[]        = {COLL_BARRIERS(COLL_DESCRIPTION, COLL_COMMA),
                                                           {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_alltoall_description[]       = {COLL_ALLTOALLS(COLL_DESCRIPTION, COLL_COMMA),
                                                            {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_alltoallv_description[]      = {COLL_ALLTOALLVS(COLL_DESCRIPTION, COLL_COMMA),
                                                             {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_bcast_description[]  = {COLL_BCASTS(COLL_DESCRIPTION, COLL_COMMA), {"", "", nullptr}};
s_mpi_coll_description_t mpi_coll_reduce_description[] = {COLL_REDUCES(COLL_DESCRIPTION, COLL_COMMA),
                                                          {"", "", nullptr}};

// Needed by the automatic selector weird implementation
s_mpi_coll_description_t* colls::get_smpi_coll_description(const char* name, int rank)
{
  if (strcmp(name, "gather") == 0)
    return &mpi_coll_gather_description[rank];
  if (strcmp(name, "allgather") == 0)
    return &mpi_coll_allgather_description[rank];
  if (strcmp(name, "allgatherv") == 0)
    return &mpi_coll_allgatherv_description[rank];
  if (strcmp(name, "allreduce") == 0)
    return &mpi_coll_allreduce_description[rank];
  if (strcmp(name, "reduce_scatter") == 0)
    return &mpi_coll_reduce_scatter_description[rank];
  if (strcmp(name, "scatter") == 0)
    return &mpi_coll_scatter_description[rank];
  if (strcmp(name, "barrier") == 0)
    return &mpi_coll_barrier_description[rank];
  if (strcmp(name, "alltoall") == 0)
    return &mpi_coll_alltoall_description[rank];
  if (strcmp(name, "alltoallv") == 0)
    return &mpi_coll_alltoallv_description[rank];
  if (strcmp(name, "bcast") == 0)
    return &mpi_coll_bcast_description[rank];
  if (strcmp(name, "reduce") == 0)
    return &mpi_coll_reduce_description[rank];
  XBT_INFO("You requested an unknown collective: %s", name);
  return nullptr;
}

/** Displays the long description of all registered models, and quit */
void colls::coll_help(const char* category, s_mpi_coll_description_t* table)
{
  XBT_WARN("Long description of the %s models accepted by this simulator:\n", category);
  for (int i = 0; not table[i].name.empty(); i++)
    XBT_WARN("  %s: %s\n", table[i].name.c_str(), table[i].description.c_str());
}

int colls::find_coll_description(s_mpi_coll_description_t* table, const std::string& name, const char* desc)
{
  for (int i = 0; not table[i].name.empty(); i++)
    if (name == table[i].name) {
      if (table[i].name != "default")
        XBT_INFO("Switch to algorithm %s for collective %s",table[i].name.c_str(),desc);
      return i;
    }

  if (table[0].name.empty())
    xbt_die("No collective is valid for '%s'! This is a bug.", name.c_str());
  std::string name_list = table[0].name;
  for (int i = 1; not table[i].name.empty(); i++)
    name_list = name_list + ", " + table[i].name;

  xbt_die("Collective '%s' is invalid! Valid collectives are: %s.", name.c_str(), name_list.c_str());
  return -1;
}

int (*colls::gather)(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff, int recv_count,
                     MPI_Datatype recv_type, int root, MPI_Comm comm);
int (*colls::allgather)(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff, int recv_count,
                        MPI_Datatype recv_type, MPI_Comm comm);
int (*colls::allgatherv)(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff,
                         const int* recv_count, const int* recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int (*colls::alltoall)(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff, int recv_count,
                       MPI_Datatype recv_type, MPI_Comm comm);
int (*colls::alltoallv)(const void* send_buff, const int* send_counts, const int* send_disps, MPI_Datatype send_type,
                        void* recv_buff, const int* recv_counts, const int* recv_disps, MPI_Datatype recv_type,
                        MPI_Comm comm);
int (*colls::bcast)(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int (*colls::reduce)(const void* buf, void* rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int (*colls::allreduce)(const void* sbuf, void* rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int (*colls::reduce_scatter)(const void* sbuf, void* rbuf, const int* rcounts, MPI_Datatype dtype, MPI_Op op,
                             MPI_Comm comm);
int (*colls::scatter)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm);
int (*colls::barrier)(MPI_Comm comm);

void (*colls::smpi_coll_cleanup_callback)();

void colls::set_gather(const std::string& name)
{
  int id = find_coll_description(mpi_coll_gather_description, name, "gather");
  gather = reinterpret_cast<int(*)(const void *send_buff, int send_count, MPI_Datatype send_type,
                                    void *recv_buff, int recv_count, MPI_Datatype recv_type,
                                        int root, MPI_Comm comm)>(mpi_coll_gather_description[id].coll);
  if (gather == nullptr)
    xbt_die("Collective gather set to nullptr!");
}

//COLL_APPLY(COLL_SETTER,COLL_GATHER_SIG,"");
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

void colls::set_collectives()
{
  std::string selector_name = simgrid::config::get_value<std::string>("smpi/coll-selector");
  if (selector_name.empty())
    selector_name = "default";

  std::pair<std::string, std::function<void(std::string)>> setter_callbacks[] = {
      {"gather", &colls::set_gather},         {"allgather", &colls::set_allgather},
      {"allgatherv", &colls::set_allgatherv}, {"allreduce", &colls::set_allreduce},
      {"alltoall", &colls::set_alltoall},     {"alltoallv", &colls::set_alltoallv},
      {"reduce", &colls::set_reduce},         {"reduce_scatter", &colls::set_reduce_scatter},
      {"scatter", &colls::set_scatter},       {"bcast", &colls::set_bcast},
      {"barrier", &colls::set_barrier}};

  for (auto& elem : setter_callbacks) {
    std::string name = simgrid::config::get_value<std::string>(("smpi/" + elem.first).c_str());
    if (name.empty())
      name = selector_name;

    (elem.second)(name);
  }
}

//Implementations of the single algorithm collectives

int colls::gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                   const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  colls::igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int colls::scatterv(const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype, void* recvbuf,
                    int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  MPI_Request request;
  colls::iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

int colls::scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
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
  MPI_Request* requests = new MPI_Request[size - 1];
  unsigned char** tmpbufs = new unsigned char*[rank];
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
  delete[] tmpbufs;
  delete[] requests;
  return MPI_SUCCESS;
}

int colls::exscan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb         = 0;
  MPI_Aint dataext    = 0;
  int recvbuf_is_empty=1;
  int rank = comm->rank();
  int size = comm->size();

  datatype->extent(&lb, &dataext);

  // Send/Recv buffers to/from others
  MPI_Request* requests = new MPI_Request[size - 1];
  unsigned char** tmpbufs = new unsigned char*[rank];
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
  delete[] tmpbufs;
  delete[] requests;
  return MPI_SUCCESS;
}

int colls::alltoallw(const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes,
                     void* recvbuf, const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes,
                     MPI_Comm comm)
{
  MPI_Request request;
  colls::ialltoallw(sendbuf, sendcounts, senddisps, sendtypes, recvbuf, recvcounts, recvdisps, recvtypes, comm,
                    &request, 0);
  return Request::wait(&request, MPI_STATUS_IGNORE);
}

}
}
