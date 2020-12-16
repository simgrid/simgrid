/* smpi_coll.c -- various optimized routing for collectives                 */

/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_coll.hpp"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"
#include "smpi_request.hpp"
#include "xbt/config.hpp"

#include <map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi, "Logging specific to SMPI collectives.");

namespace simgrid {
namespace smpi {

std::map<std::string, std::vector<s_mpi_coll_description_t>, std::less<>> smpi_coll_descriptions(
    {{std::string("gather"),
      {{"default", "gather default collective", (void*)gather__default},
       {"ompi", "gather ompi collective", (void*)gather__ompi},
       {"ompi_basic_linear", "gather ompi_basic_linear collective", (void*)gather__ompi_basic_linear},
       {"ompi_binomial", "gather ompi_binomial collective", (void*)gather__ompi_binomial},
       {"ompi_linear_sync", "gather ompi_linear_sync collective", (void*)gather__ompi_linear_sync},
       {"mpich", "gather mpich collective", (void*)gather__mpich},
       {"mvapich2", "gather mvapich2 collective", (void*)gather__mvapich2},
       {"mvapich2_two_level", "gather mvapich2_two_level collective", (void*)gather__mvapich2_two_level},
       {"impi", "gather impi collective", (void*)gather__impi},
       {"automatic", "gather automatic collective", (void*)gather__automatic}}},

     {"allgather",
      {{"default", "allgather default collective", (void*)allgather__default},
       {"2dmesh", "allgather 2dmesh collective", (void*)allgather__2dmesh},
       {"3dmesh", "allgather 3dmesh collective", (void*)allgather__3dmesh},
       {"bruck", "allgather bruck collective", (void*)allgather__bruck},
       {"GB", "allgather GB collective", (void*)allgather__GB},
       {"loosely_lr", "allgather loosely_lr collective", (void*)allgather__loosely_lr},
       {"NTSLR", "allgather NTSLR collective", (void*)allgather__NTSLR},
       {"NTSLR_NB", "allgather NTSLR_NB collective", (void*)allgather__NTSLR_NB},
       {"pair", "allgather pair collective", (void*)allgather__pair},
       {"rdb", "allgather rdb collective", (void*)allgather__rdb},
       {"rhv", "allgather rhv collective", (void*)allgather__rhv},
       {"ring", "allgather ring collective", (void*)allgather__ring},
       {"SMP_NTS", "allgather SMP_NTS collective", (void*)allgather__SMP_NTS},
       {"smp_simple", "allgather smp_simple collective", (void*)allgather__smp_simple},
       {"spreading_simple", "allgather spreading_simple collective", (void*)allgather__spreading_simple},
       {"ompi", "allgather ompi collective", (void*)allgather__ompi},
       {"ompi_neighborexchange", "allgather ompi_neighborexchange collective", (void*)allgather__ompi_neighborexchange},
       {"mvapich2", "allgather mvapich2 collective", (void*)allgather__mvapich2},
       {"mvapich2_smp", "allgather mvapich2_smp collective", (void*)allgather__mvapich2_smp},
       {"mpich", "allgather mpich collective", (void*)allgather__mpich},
       {"impi", "allgather impi collective", (void*)allgather__impi},
       {"automatic", "allgather automatic collective", (void*)allgather__automatic}}},

     {"allgatherv",
      {{"default", "allgatherv default collective", (void*)allgatherv__default},
       {"GB", "allgatherv GB collective", (void*)allgatherv__GB},
       {"pair", "allgatherv pair collective", (void*)allgatherv__pair},
       {"ring", "allgatherv ring collective", (void*)allgatherv__ring},
       {"ompi", "allgatherv ompi collective", (void*)allgatherv__ompi},
       {"ompi_neighborexchange", "allgatherv ompi_neighborexchange collective",
        (void*)allgatherv__ompi_neighborexchange},
       {"ompi_bruck", "allgatherv ompi_bruck collective", (void*)allgatherv__ompi_bruck},
       {"mpich", "allgatherv mpich collective", (void*)allgatherv__mpich},
       {"mpich_rdb", "allgatherv mpich_rdb collective", (void*)allgatherv__mpich_rdb},
       {"mpich_ring", "allgatherv mpich_ring collective", (void*)allgatherv__mpich_ring},
       {"mvapich2", "allgatherv mvapich2 collective", (void*)allgatherv__mvapich2},
       {"impi", "allgatherv impi collective", (void*)allgatherv__impi},
       {"automatic", "allgatherv automatic collective", (void*)allgatherv__automatic}}},

     {"allreduce",
      {{"default", "allreduce default collective", (void*)allreduce__default},
       {"lr", "allreduce lr collective", (void*)allreduce__lr},
       {"rab1", "allreduce rab1 collective", (void*)allreduce__rab1},
       {"rab2", "allreduce rab2 collective", (void*)allreduce__rab2},
       {"rab_rdb", "allreduce rab_rdb collective", (void*)allreduce__rab_rdb},
       {"rdb", "allreduce rdb collective", (void*)allreduce__rdb},
       {"smp_binomial", "allreduce smp_binomial collective", (void*)allreduce__smp_binomial},
       {"smp_binomial_pipeline", "allreduce smp_binomial_pipeline collective", (void*)allreduce__smp_binomial_pipeline},
       {"smp_rdb", "allreduce smp_rdb collective", (void*)allreduce__smp_rdb},
       {"smp_rsag", "allreduce smp_rsag collective", (void*)allreduce__smp_rsag},
       {"smp_rsag_lr", "allreduce smp_rsag_lr collective", (void*)allreduce__smp_rsag_lr},
       {"smp_rsag_rab", "allreduce smp_rsag_rab collective", (void*)allreduce__smp_rsag_rab},
       {"redbcast", "allreduce redbcast collective", (void*)allreduce__redbcast},
       {"ompi", "allreduce ompi collective", (void*)allreduce__ompi},
       {"ompi_ring_segmented", "allreduce ompi_ring_segmented collective", (void*)allreduce__ompi_ring_segmented},
       {"mpich", "allreduce mpich collective", (void*)allreduce__mpich},
       {"mvapich2", "allreduce mvapich2 collective", (void*)allreduce__mvapich2},
       {"mvapich2_rs", "allreduce mvapich2_rs collective", (void*)allreduce__mvapich2_rs},
       {"mvapich2_two_level", "allreduce mvapich2_two_level collective", (void*)allreduce__mvapich2_two_level},
       {"impi", "allreduce impi collective", (void*)allreduce__impi},
       {"rab", "allreduce rab collective", (void*)allreduce__rab},
       {"automatic", "allreduce automatic collective", (void*)allreduce__automatic}}},

     {"reduce_scatter",
      {{"default", "reduce_scatter default collective", (void*)reduce_scatter__default},
       {"ompi", "reduce_scatter ompi collective", (void*)reduce_scatter__ompi},
       {"ompi_basic_recursivehalving", "reduce_scatter ompi_basic_recursivehalving collective",
        (void*)reduce_scatter__ompi_basic_recursivehalving},
       {"ompi_ring", "reduce_scatter ompi_ring collective", (void*)reduce_scatter__ompi_ring},
       {"mpich", "reduce_scatter mpich collective", (void*)reduce_scatter__mpich},
       {"mpich_pair", "reduce_scatter mpich_pair collective", (void*)reduce_scatter__mpich_pair},
       {"mpich_rdb", "reduce_scatter mpich_rdb collective", (void*)reduce_scatter__mpich_rdb},
       {"mpich_noncomm", "reduce_scatter mpich_noncomm collective", (void*)reduce_scatter__mpich_noncomm},
       {"mvapich2", "reduce_scatter mvapich2 collective", (void*)reduce_scatter__mvapich2},
       {"impi", "reduce_scatter impi collective", (void*)reduce_scatter__impi},
       {"automatic", "reduce_scatter automatic collective", (void*)reduce_scatter__automatic}}},

     {"scatter",
      {{"default", "scatter default collective", (void*)scatter__default},
       {"ompi", "scatter ompi collective", (void*)scatter__ompi},
       {"ompi_basic_linear", "scatter ompi_basic_linear collective", (void*)scatter__ompi_basic_linear},
       {"ompi_binomial", "scatter ompi_binomial collective", (void*)scatter__ompi_binomial},
       {"mpich", "scatter mpich collective", (void*)scatter__mpich},
       {"mvapich2", "scatter mvapich2 collective", (void*)scatter__mvapich2},
       {"mvapich2_two_level_binomial", "scatter mvapich2_two_level_binomial collective",
        (void*)scatter__mvapich2_two_level_binomial},
       {"mvapich2_two_level_direct", "scatter mvapich2_two_level_direct collective",
        (void*)scatter__mvapich2_two_level_direct},
       {"impi", "scatter impi collective", (void*)scatter__impi},
       {"automatic", "scatter automatic collective", (void*)scatter__automatic}}},

     {"barrier",
      {{"default", "barrier default collective", (void*)barrier__default},
       {"ompi", "barrier ompi collective", (void*)barrier__ompi},
       {"ompi_basic_linear", "barrier ompi_basic_linear collective", (void*)barrier__ompi_basic_linear},
       {"ompi_two_procs", "barrier ompi_two_procs collective", (void*)barrier__ompi_two_procs},
       {"ompi_tree", "barrier ompi_tree collective", (void*)barrier__ompi_tree},
       {"ompi_bruck", "barrier ompi_bruck collective", (void*)barrier__ompi_bruck},
       {"ompi_recursivedoubling", "barrier ompi_recursivedoubling collective", (void*)barrier__ompi_recursivedoubling},
       {"ompi_doublering", "barrier ompi_doublering collective", (void*)barrier__ompi_doublering},
       {"mpich_smp", "barrier mpich_smp collective", (void*)barrier__mpich_smp},
       {"mpich", "barrier mpich collective", (void*)barrier__mpich},
       {"mvapich2_pair", "barrier mvapich2_pair collective", (void*)barrier__mvapich2_pair},
       {"mvapich2", "barrier mvapich2 collective", (void*)barrier__mvapich2},
       {"impi", "barrier impi collective", (void*)barrier__impi},
       {"automatic", "barrier automatic collective", (void*)barrier__automatic}}},

     {"alltoall",
      {{"default", "alltoall default collective", (void*)alltoall__default},
       {"2dmesh", "alltoall 2dmesh collective", (void*)alltoall__2dmesh},
       {"3dmesh", "alltoall 3dmesh collective", (void*)alltoall__3dmesh},
       {"basic_linear", "alltoall basic_linear collective", (void*)alltoall__basic_linear},
       {"bruck", "alltoall bruck collective", (void*)alltoall__bruck},
       {"pair", "alltoall pair collective", (void*)alltoall__pair},
       {"pair_rma", "alltoall pair_rma collective", (void*)alltoall__pair_rma},
       {"pair_light_barrier", "alltoall pair_light_barrier collective", (void*)alltoall__pair_light_barrier},
       {"pair_mpi_barrier", "alltoall pair_mpi_barrier collective", (void*)alltoall__pair_mpi_barrier},
       {"pair_one_barrier", "alltoall pair_one_barrier collective", (void*)alltoall__pair_one_barrier},
       {"rdb", "alltoall rdb collective", (void*)alltoall__rdb},
       {"ring", "alltoall ring collective", (void*)alltoall__ring},
       {"ring_light_barrier", "alltoall ring_light_barrier collective", (void*)alltoall__ring_light_barrier},
       {"ring_mpi_barrier", "alltoall ring_mpi_barrier collective", (void*)alltoall__ring_mpi_barrier},
       {"ring_one_barrier", "alltoall ring_one_barrier collective", (void*)alltoall__ring_one_barrier},
       {"mvapich2", "alltoall mvapich2 collective", (void*)alltoall__mvapich2},
       {"mvapich2_scatter_dest", "alltoall mvapich2_scatter_dest collective", (void*)alltoall__mvapich2_scatter_dest},
       {"ompi", "alltoall ompi collective", (void*)alltoall__ompi},
       {"mpich", "alltoall mpich collective", (void*)alltoall__mpich},
       {"impi", "alltoall impi collective", (void*)alltoall__impi},
       {"automatic", "alltoall automatic collective", (void*)alltoall__automatic}}},

     {"alltoallv",
      {{"default", "alltoallv default collective", (void*)alltoallv__default},
       {"bruck", "alltoallv bruck collective", (void*)alltoallv__bruck},
       {"pair", "alltoallv pair collective", (void*)alltoallv__pair},
       {"pair_light_barrier", "alltoallv pair_light_barrier collective", (void*)alltoallv__pair_light_barrier},
       {"pair_mpi_barrier", "alltoallv pair_mpi_barrier collective", (void*)alltoallv__pair_mpi_barrier},
       {"pair_one_barrier", "alltoallv pair_one_barrier collective", (void*)alltoallv__pair_one_barrier},
       {"ring", "alltoallv ring collective", (void*)alltoallv__ring},
       {"ring_light_barrier", "alltoallv ring_light_barrier collective", (void*)alltoallv__ring_light_barrier},
       {"ring_mpi_barrier", "alltoallv ring_mpi_barrier collective", (void*)alltoallv__ring_mpi_barrier},
       {"ring_one_barrier", "alltoallv ring_one_barrier collective", (void*)alltoallv__ring_one_barrier},
       {"ompi", "alltoallv ompi collective", (void*)alltoallv__ompi},
       {"mpich", "alltoallv mpich collective", (void*)alltoallv__mpich},
       {"ompi_basic_linear", "alltoallv ompi_basic_linear collective", (void*)alltoallv__ompi_basic_linear},
       {"mvapich2", "alltoallv mvapich2 collective", (void*)alltoallv__mvapich2},
       {"impi", "alltoallv impi collective", (void*)alltoallv__impi},
       {"automatic", "alltoallv automatic collective", (void*)alltoallv__automatic}}},

     {"bcast",
      {{"default", "bcast default collective", (void*)bcast__default},
       {"arrival_pattern_aware", "bcast arrival_pattern_aware collective", (void*)bcast__arrival_pattern_aware},
       {"arrival_pattern_aware_wait", "bcast arrival_pattern_aware_wait collective",
        (void*)bcast__arrival_pattern_aware_wait},
       {"arrival_scatter", "bcast arrival_scatter collective", (void*)bcast__arrival_scatter},
       {"binomial_tree", "bcast binomial_tree collective", (void*)bcast__binomial_tree},
       {"flattree", "bcast flattree collective", (void*)bcast__flattree},
       {"flattree_pipeline", "bcast flattree_pipeline collective", (void*)bcast__flattree_pipeline},
       {"NTSB", "bcast NTSB collective", (void*)bcast__NTSB},
       {"NTSL", "bcast NTSL collective", (void*)bcast__NTSL},
       {"NTSL_Isend", "bcast NTSL_Isend collective", (void*)bcast__NTSL_Isend},
       {"scatter_LR_allgather", "bcast scatter_LR_allgather collective", (void*)bcast__scatter_LR_allgather},
       {"scatter_rdb_allgather", "bcast scatter_rdb_allgather collective", (void*)bcast__scatter_rdb_allgather},
       {"SMP_binary", "bcast SMP_binary collective", (void*)bcast__SMP_binary},
       {"SMP_binomial", "bcast SMP_binomial collective", (void*)bcast__SMP_binomial},
       {"SMP_linear", "bcast SMP_linear collective", (void*)bcast__SMP_linear},
       {"ompi", "bcast ompi collective", (void*)bcast__ompi},
       {"ompi_split_bintree", "bcast ompi_split_bintree collective", (void*)bcast__ompi_split_bintree},
       {"ompi_pipeline", "bcast ompi_pipeline collective", (void*)bcast__ompi_pipeline},
       {"mpich", "bcast mpich collective", (void*)bcast__mpich},
       {"mvapich2", "bcast mvapich2 collective", (void*)bcast__mvapich2},
       {"mvapich2_inter_node", "bcast mvapich2_inter_node collective", (void*)bcast__mvapich2_inter_node},
       {"mvapich2_intra_node", "bcast mvapich2_intra_node collective", (void*)bcast__mvapich2_intra_node},
       {"mvapich2_knomial_intra_node", "bcast mvapich2_knomial_intra_node collective",
        (void*)bcast__mvapich2_knomial_intra_node},
       {"impi", "bcast impi collective", (void*)bcast__impi},
       {"automatic", "bcast automatic collective", (void*)bcast__automatic}}},

     {"reduce",
      {{"default", "reduce default collective", (void*)reduce__default},
       {"arrival_pattern_aware", "reduce arrival_pattern_aware collective", (void*)reduce__arrival_pattern_aware},
       {"binomial", "reduce binomial collective", (void*)reduce__binomial},
       {"flat_tree", "reduce flat_tree collective", (void*)reduce__flat_tree},
       {"NTSL", "reduce NTSL collective", (void*)reduce__NTSL},
       {"scatter_gather", "reduce scatter_gather collective", (void*)reduce__scatter_gather},
       {"ompi", "reduce ompi collective", (void*)reduce__ompi},
       {"ompi_chain", "reduce ompi_chain collective", (void*)reduce__ompi_chain},
       {"ompi_pipeline", "reduce ompi_pipeline collective", (void*)reduce__ompi_pipeline},
       {"ompi_basic_linear", "reduce ompi_basic_linear collective", (void*)reduce__ompi_basic_linear},
       {"ompi_in_order_binary", "reduce ompi_in_order_binary collective", (void*)reduce__ompi_in_order_binary},
       {"ompi_binary", "reduce ompi_binary collective", (void*)reduce__ompi_binary},
       {"ompi_binomial", "reduce ompi_binomial collective", (void*)reduce__ompi_binomial},
       {"mpich", "reduce mpich collective", (void*)reduce__mpich},
       {"mvapich2", "reduce mvapich2 collective", (void*)reduce__mvapich2},
       {"mvapich2_knomial", "reduce mvapich2_knomial collective", (void*)reduce__mvapich2_knomial},
       {"mvapich2_two_level", "reduce mvapich2_two_level collective", (void*)reduce__mvapich2_two_level},
       {"impi", "reduce impi collective", (void*)reduce__impi},
       {"rab", "reduce rab collective", (void*)reduce__rab},
       {"automatic", "reduce automatic collective", (void*)reduce__automatic}}}});

// Needed by the automatic selector weird implementation
std::vector<s_mpi_coll_description_t>* colls::get_smpi_coll_descriptions(const std::string& name)
{
  auto iter = smpi_coll_descriptions.find(name);
  if (iter == smpi_coll_descriptions.end())
    xbt_die("No collective named %s. This is a bug.", name.c_str());
  return &iter->second;
}

static s_mpi_coll_description_t* find_coll_description(const std::string& collective, const std::string& algo)
{
  std::vector<s_mpi_coll_description_t>* table = colls::get_smpi_coll_descriptions(collective);
  if (table->empty())
    xbt_die("No registered algorithm for collective '%s'! This is a bug.", collective.c_str());

  for (auto& desc : *table) {
    if (algo == desc.name) {
      if (desc.name != "default")
        XBT_INFO("Switch to algorithm %s for collective %s", desc.name.c_str(), collective.c_str());
      return &desc;
    }
  }

  std::string name_list = table->at(0).name;
  for (unsigned long i = 1; i < table->size(); i++)
    name_list = name_list + ", " + table->at(i).name;
  xbt_die("Collective '%s' has no algorithm '%s'! Valid algorithms: %s.", collective.c_str(), algo.c_str(), name_list.c_str());
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

#define COLL_SETTER(cat, ret, args, args2)                                                                             \
  void colls::_XBT_CONCAT(set_, cat)(const std::string& name)                                                          \
  {                                                                                                                    \
    auto desc = find_coll_description(_XBT_STRINGIFY(cat), name);                                                      \
    cat       = reinterpret_cast<ret(*) args>(desc->coll);                                                             \
    if (cat == nullptr)                                                                                                \
      xbt_die("Collective " _XBT_STRINGIFY(cat) " set to nullptr!");                                                   \
  }
COLL_APPLY(COLL_SETTER, COLL_GATHER_SIG, "")
COLL_APPLY(COLL_SETTER,COLL_ALLGATHER_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_ALLGATHERV_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_REDUCE_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_ALLREDUCE_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_REDUCE_SCATTER_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_SCATTER_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_BARRIER_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_BCAST_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_ALLTOALL_SIG,"")
COLL_APPLY(COLL_SETTER,COLL_ALLTOALLV_SIG,"")

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
  auto* requests = new MPI_Request[size - 1];
  auto** tmpbufs = new unsigned char*[rank];
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
        // #Request is below rank: it's an irecv
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
  auto* requests = new MPI_Request[size - 1];
  auto** tmpbufs = new unsigned char*[rank];
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
          // #Request is below rank: it's an irecv
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
