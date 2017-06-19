/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/smpi/private.h"
#include "src/smpi/smpi_comm.hpp"
#include "src/smpi/smpi_coll.hpp"
#include "src/smpi/smpi_datatype_derived.hpp"
#include "src/smpi/smpi_op.hpp"
#include "src/smpi/smpi_process.hpp"
#include "src/smpi/smpi_request.hpp"
#include "src/smpi/smpi_status.hpp"
#include "src/smpi/smpi_win.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi, "Logging specific to SMPI (pmpi)");

//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  TRACE_internal_smpi_set_category (category);
  //begin bench after changing process's category
  smpi_bench_begin();
}

/* PMPI User level calls */
extern "C" { // Obviously, the C MPI interface should use the C linkage

int PMPI_Init(int *argc, char ***argv)
{
  xbt_assert(simgrid::s4u::Engine::isInitialized(),
             "Your MPI program was not properly initialized. The easiest is to use smpirun to start it.");
  // PMPI_Init is called only once per SMPI process
  int already_init;
  MPI_Initialized(&already_init);
  if(already_init == 0){
    simgrid::smpi::Process::init(argc, argv);
    smpi_process()->mark_as_initialized();
    int rank = smpi_process()->index();
    TRACE_smpi_init(rank);
    TRACE_smpi_computing_init(rank);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_INIT;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
    smpi_bench_begin();
  }

  smpi_mpi_init();

  return MPI_SUCCESS;
}

int PMPI_Finalize()
{
  smpi_bench_end();
  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_FINALIZE;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  smpi_process()->finalize();

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_finalize(smpi_process()->index());
  return MPI_SUCCESS;
}

int PMPI_Finalized(int* flag)
{
  *flag=smpi_process()!=nullptr ? smpi_process()->finalized() : 0;
  return MPI_SUCCESS;
}

int PMPI_Get_version (int *version,int *subversion){
  *version = MPI_VERSION;
  *subversion= MPI_SUBVERSION;
  return MPI_SUCCESS;
}

int PMPI_Get_library_version (char *version,int *len){
  smpi_bench_end();
  snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING, "SMPI Version %d.%d. Copyright The Simgrid Team 2007-2017",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR);
  *len = strlen(version) > MPI_MAX_LIBRARY_VERSION_STRING ? MPI_MAX_LIBRARY_VERSION_STRING : strlen(version);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  if (provided != nullptr) {
    *provided = MPI_THREAD_SINGLE;
  }
  return MPI_Init(argc, argv);
}

int PMPI_Query_thread(int *provided)
{
  if (provided == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_SINGLE;
    return MPI_SUCCESS;
  }
}

int PMPI_Is_thread_main(int *flag)
{
  if (flag == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *flag = smpi_process()->index() == 0;
    return MPI_SUCCESS;
  }
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
  smpi_bench_end();
  // FIXME: should kill all processes in comm instead
  simcall_process_kill(SIMIX_process_self());
  return MPI_SUCCESS;
}

double PMPI_Wtime()
{
  return smpi_mpi_wtime();
}

extern double sg_maxmin_precision;
double PMPI_Wtick()
{
  return sg_maxmin_precision;
}

int PMPI_Address(void *location, MPI_Aint * address)
{
  if (address==nullptr) {
    return MPI_ERR_ARG;
  } else {
    *address = reinterpret_cast<MPI_Aint>(location);
    return MPI_SUCCESS;
  }
}

int PMPI_Get_address(void *location, MPI_Aint * address)
{
  return PMPI_Address(location, address);
}

int PMPI_Type_free(MPI_Datatype * datatype)
{
  /* Free a predefined datatype is an error according to the standard, and should be checked for */
  if (*datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_ARG;
  } else {
    simgrid::smpi::Datatype::unref(*datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<int>(datatype->size());
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<MPI_Count>(datatype->size());
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (lb == nullptr || extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return datatype->extent(lb, extent);
  }
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *extent = datatype->get_extent();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = datatype->lb();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = datatype->ub();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype){
  int retval = MPI_SUCCESS;
  if (datatype == MPI_DATATYPE_NULL) {
    retval=MPI_ERR_TYPE;
  } else {
    *newtype = new simgrid::smpi::Datatype(datatype, &retval);
    //error when duplicating, free the new datatype
    if(retval!=MPI_SUCCESS){
      simgrid::smpi::Datatype::unref(*newtype);
      *newtype = MPI_DATATYPE_NULL;
    }
  }
  return retval;
}

int PMPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  if (function == nullptr || op == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *op = new simgrid::smpi::Op(function, (commute!=0));
    return MPI_SUCCESS;
  }
}

int PMPI_Op_free(MPI_Op * op)
{
  if (op == nullptr) {
    return MPI_ERR_ARG;
  } else if (*op == MPI_OP_NULL) {
    return MPI_ERR_OP;
  } else {
    delete (*op);
    *op = MPI_OP_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Op_commutative(MPI_Op op, int* commute){
  if (op == MPI_OP_NULL) {
    return MPI_ERR_OP;
  } else if (commute==nullptr){
    return MPI_ERR_ARG;
  } else {
    *commute = op->is_commutative();
    return MPI_SUCCESS;
  }
}

int PMPI_Group_free(MPI_Group * group)
{
  if (group == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if(*group != MPI_COMM_WORLD->group() && *group != MPI_GROUP_EMPTY)
      simgrid::smpi::Group::unref(*group);
    *group = MPI_GROUP_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Group_size(MPI_Group group, int *size)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = group->size();
    return MPI_SUCCESS;
  }
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (rank == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *rank = group->rank(smpi_process()->index());
    return MPI_SUCCESS;
  }
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else {
    for (int i = 0; i < n; i++) {
      if(ranks1[i]==MPI_PROC_NULL){
        ranks2[i]=MPI_PROC_NULL;
      }else{
        int index = group1->index(ranks1[i]);
        ranks2[i] = group2->rank(index);
      }
    }
    return MPI_SUCCESS;
  }
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (result == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *result = group1->compare(group2);
    return MPI_SUCCESS;
  }
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->group_union(group2, newgroup);
  }
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->intersection(group2,newgroup);
  }
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->difference(group2,newgroup);
  }
}

int PMPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group->incl(n, ranks, newgroup);
  }
}

int PMPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if (group != MPI_COMM_WORLD->group()
                && group != MPI_COMM_SELF->group() && group != MPI_GROUP_EMPTY)
      group->ref();
      return MPI_SUCCESS;
    } else if (n == group->size()) {
      *newgroup = MPI_GROUP_EMPTY;
      return MPI_SUCCESS;
    } else {
      return group->excl(n,ranks,newgroup);
    }
  }
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
      return MPI_SUCCESS;
    } else {
      return group->range_incl(n,ranges,newgroup);
    }
  }
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if (group != MPI_COMM_WORLD->group() && group != MPI_COMM_SELF->group() &&
          group != MPI_GROUP_EMPTY)
        group->ref();
      return MPI_SUCCESS;
    } else {
      return group->range_excl(n,ranges,newgroup);
    }
  }
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (rank == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *rank = comm->rank();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = comm->size();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  if (comm == MPI_COMM_NULL)  {
    return MPI_ERR_COMM;
  } else if (name == nullptr || len == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    comm->get_name(name, len);
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (group == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *group = comm->group();
    if (*group != MPI_COMM_WORLD->group() && *group != MPI_GROUP_NULL && *group != MPI_GROUP_EMPTY)
      (*group)->ref();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  if (comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (result == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (comm1 == comm2) {       /* Same communicators means same groups */
      *result = MPI_IDENT;
    } else {
      *result = comm1->group()->compare(comm2->group());
      if (*result == MPI_IDENT) {
        *result = MPI_CONGRUENT;
      }
    }
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (newcomm == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return comm->dup(newcomm);
  }
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newcomm == nullptr) {
    return MPI_ERR_ARG;
  } else if(group->rank(smpi_process()->index())==MPI_UNDEFINED){
    *newcomm= MPI_COMM_NULL;
    return MPI_SUCCESS;
  }else{
    group->ref();
    *newcomm = new simgrid::smpi::Comm(group, nullptr);
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_free(MPI_Comm * comm)
{
  if (comm == nullptr) {
    return MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else {
    simgrid::smpi::Comm::destroy(*comm);
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
  /* TODO: wait until all communication in comm are done */
  if (comm == nullptr) {
    return MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else {
    simgrid::smpi::Comm::destroy(*comm);
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  int retval = 0;
  smpi_bench_end();

  if (comm_out == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *comm_out = comm->split(color, key);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();

  return retval;
}

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int, MPI_Comm* comm_out)
{
  int retval = 0;
  smpi_bench_end();

  if (comm_out == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    retval = MPI_Comm_create(comm, group, comm_out);
  }
  smpi_bench_begin();

  return retval;
}

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
      retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
      retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
      retval = MPI_SUCCESS;
  } else {
      *request = simgrid::smpi::Request::send_init(buf, count, datatype, dst, tag, comm);
      retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = simgrid::smpi::Request::recv_init(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = simgrid::smpi::Request::ssend_init(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

static int persistent_send(MPI_Request *request)
{
  int retval = 0;
  MPI_Request req = *request;

  int rank = req->comm() != MPI_COMM_NULL ? smpi_process_index() : -1;
  int dst_traced = req->comm()->group()->index(req->dst()); //smpi_group_index(smpi_comm_group(req->comm()), req->dst());

  int known = 0;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ISEND;
  extra->send_size = req->size() / req->old_type()->size();
  extra->src = rank;
  extra->dst = dst_traced;
  extra->tag = req->tag();
  extra->datatype1 = encode_datatype(req->old_type(), &known);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  TRACE_smpi_send(rank, rank, dst_traced, req->tag(), req->size());

  (*request)->start();
  retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);

  return retval;
}

static int persistent_recv(MPI_Request *request)
{

  int retval = 0;
  MPI_Request req = *request;

  int rank = req->comm() != MPI_COMM_NULL ? smpi_process_index() : -1;
  int src_traced = req->comm()->group()->index(req->src()); //smpi_group_index(smpi_comm_group(req->comm), req->src);

  int known = 0;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_IRECV;
  extra->send_size = req->size() / req->old_type()->size();
  extra->src = src_traced;
  extra->dst = rank;
  extra->tag = req->tag();
  extra->datatype1 = encode_datatype(req->old_type(), &known);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

  (*request)->start();
  retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);

  return retval;
}

int PMPI_Start(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr || *request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
      if((*request)->flags() & SEND){
      retval = persistent_send(request);
    }else if((*request)->flags() & RECV){
      retval = persistent_recv(request);
    }else{
      (*request)->start();
      retval = MPI_SUCCESS;
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Startall(int count, MPI_Request * requests)
{
  int retval;
  smpi_bench_end();
  if (requests == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    retval = MPI_SUCCESS;
    for (int i = 0; i < count; i++) {
      if(requests[i] == MPI_REQUEST_NULL) {
        retval = MPI_ERR_REQUEST;
      }
    }
    if(retval != MPI_ERR_REQUEST) {
      simgrid::smpi::Request::startall(count, requests);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    simgrid::smpi::Request::unref(request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int rank = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int src_traced = comm->group()->index(src);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_IRECV;
    extra->src = src_traced;
    extra->dst = rank;
    extra->tag = tag;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = datatype->size();
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    *request = simgrid::smpi::Request::irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}


int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int dst_traced = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISEND;
    extra->src = rank;
    extra->dst = dst_traced;
    extra->tag = tag;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = datatype->size();
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag, count*datatype->size());

    *request = simgrid::smpi::Request::isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Issend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0)|| (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int dst_traced = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISSEND;
    extra->src = rank;
    extra->dst = dst_traced;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = datatype->size();
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag, count*datatype->size());

    *request = simgrid::smpi::Request::issend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int src_traced         = comm->group()->index(src);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_RECV;
    extra->src             = src_traced;
    extra->dst             = rank;
    extra->tag 		   = tag;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    simgrid::smpi::Request::recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;

    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    if (status != MPI_STATUS_IGNORE) {
      src_traced = comm->group()->index(status->MPI_SOURCE);
      if (not TRACE_smpi_view_internals()) {
        TRACE_smpi_recv(rank, src_traced, rank, tag);
      }
    }
    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf == nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag < 0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int dst_traced         = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type            = TRACING_SEND;
    extra->src             = rank;
    extra->dst             = dst_traced;
    extra->tag = tag;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0) {
      dt_size_send = datatype->size();
    }
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    if (not TRACE_smpi_view_internals()) {
      TRACE_smpi_send(rank, rank, dst_traced, tag,count*datatype->size());
    }

    simgrid::smpi::Request::send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int dst_traced         = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type            = TRACING_SSEND;
    extra->src             = rank;
    extra->dst             = dst_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if(known == 0) {
      dt_size_send = datatype->size();
    }
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag,count*datatype->size());
  
    simgrid::smpi::Request::ssend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  
    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not sendtype->is_valid() || not recvtype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL || dst == MPI_PROC_NULL) {
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval             = MPI_SUCCESS;
  }else if (dst >= comm->group()->size() || dst <0 ||
      (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0))){
    retval = MPI_ERR_RANK;
  } else if ((sendcount < 0 || recvcount<0) || 
      (sendbuf==nullptr && sendcount > 0) || (recvbuf==nullptr && recvcount>0)) {
    retval = MPI_ERR_COUNT;
  } else if((sendtag<0 && sendtag !=  MPI_ANY_TAG)||(recvtag<0 && recvtag != MPI_ANY_TAG)){
    retval = MPI_ERR_TAG;
  } else {

  int rank = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
  int dst_traced = comm->group()->index(dst);
  int src_traced = comm->group()->index(src);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SENDRECV;
  extra->src = src_traced;
  extra->dst = dst_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(known==0)
    dt_size_send = sendtype->size();
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(known==0)
    dt_size_recv = recvtype->size();
  extra->recv_size = recvcount*dt_size_recv;

  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);
  TRACE_smpi_send(rank, rank, dst_traced, sendtag,sendcount*sendtype->size());

  simgrid::smpi::Request::sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag, comm,
                    status);
  retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  TRACE_smpi_recv(rank, src_traced, rank, recvtag);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv_replace(void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                          MPI_Comm comm, MPI_Status* status)
{
  int retval = 0;
  if (not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else if (count < 0) {
    return MPI_ERR_COUNT;
  } else {
    int size = datatype->get_extent() * count;
    void* recvbuf = xbt_new0(char, size);
    retval = MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count, datatype, src, recvtag, comm, status);
    if(retval==MPI_SUCCESS){
        simgrid::smpi::Datatype::copy(recvbuf, count, datatype, buf, count, datatype);
    }
    xbt_free(recvbuf);

  }
  return retval;
}

int PMPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  int retval = 0;
  smpi_bench_end();
  if (request == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    *flag= true;
    simgrid::smpi::Status::empty(status);
    retval = MPI_SUCCESS;
  } else {
    int rank = ((*request)->comm() != MPI_COMM_NULL) ? smpi_process()->index() : -1;

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_TEST;
    TRACE_smpi_testing_in(rank, extra);

    *flag = simgrid::smpi::Request::test(request,status);

    TRACE_smpi_testing_out(rank);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (index == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = simgrid::smpi::Request::testany(count, requests, index, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
  int retval = 0;

  smpi_bench_end();
  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = simgrid::smpi::Request::testall(count, requests, statuses);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (status == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    simgrid::smpi::Request::probe(source, tag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    *flag=true;
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    simgrid::smpi::Request::iprobe(source, tag, comm, flag, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  simgrid::smpi::Status::empty(status);

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_SUCCESS;
  } else {

    int rank = (request!=nullptr && (*request)->comm() != MPI_COMM_NULL) ? smpi_process()->index() : -1;

    int src_traced = (*request)->src();
    int dst_traced = (*request)->dst();
    int tag_traced= (*request)->tag();
    MPI_Comm comm = (*request)->comm();
    int is_wait_for_receive = ((*request)->flags() & RECV);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_WAIT;
    extra->src = src_traced;
    extra->dst = dst_traced;
    extra->tag = (*request)->tag();
    TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);

    simgrid::smpi::Request::wait(request, status);
    retval = MPI_SUCCESS;

    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
    if (is_wait_for_receive) {
      if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUS_IGNORE) ?
          comm->group()->rank(status->MPI_SOURCE) :
          src_traced;
      TRACE_smpi_recv(rank, src_traced, dst_traced, tag_traced);
    }
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  if (index == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  //save requests information for tracing
  typedef struct {
    int src;
    int dst;
    int recv;
    int tag;
    MPI_Comm comm;
  } savedvalstype;
  savedvalstype* savedvals=nullptr;
  if(count>0){
    savedvals = xbt_new0(savedvalstype, count);
  }
  for (int i = 0; i < count; i++) {
    MPI_Request req = requests[i];      //already received requests are no longer valid
    if (req) {
      savedvals[i]=(savedvalstype){req->src(), req->dst(), (req->flags() & RECV), req->tag(), req->comm()};
    }
  }
  int rank_traced = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITANY;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  *index = simgrid::smpi::Request::waitany(count, requests, status);

  if(*index!=MPI_UNDEFINED){
    int src_traced = savedvals[*index].src;
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int dst_traced = savedvals[*index].dst;
    int is_wait_for_receive = savedvals[*index].recv;
    if (is_wait_for_receive) {
      if(savedvals[*index].src==MPI_ANY_SOURCE)
        src_traced = (status != MPI_STATUSES_IGNORE)
                         ? savedvals[*index].comm->group()->rank(status->MPI_SOURCE)
                         : savedvals[*index].src;
      TRACE_smpi_recv(rank_traced, src_traced, dst_traced, savedvals[*index].tag);
    }
    TRACE_smpi_ptp_out(rank_traced, src_traced, dst_traced, __FUNCTION__);
  }
  xbt_free(savedvals);

  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  smpi_bench_end();
  //save information from requests
  typedef struct {
    int src;
    int dst;
    int recv;
    int tag;
    int valid;
    MPI_Comm comm;
  } savedvalstype;
  savedvalstype* savedvals=xbt_new0(savedvalstype, count);

  for (int i = 0; i < count; i++) {
    MPI_Request req = requests[i];
    if(req!=MPI_REQUEST_NULL){
      savedvals[i]=(savedvalstype){req->src(), req->dst(), (req->flags() & RECV), req->tag(), 1, req->comm()};
    }else{
      savedvals[i].valid=0;
    }
  }
  int rank_traced = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITALL;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  int retval = simgrid::smpi::Request::waitall(count, requests, status);

  for (int i = 0; i < count; i++) {
    if(savedvals[i].valid){
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
      int src_traced = savedvals[i].src;
      int dst_traced = savedvals[i].dst;
      int is_wait_for_receive = savedvals[i].recv;
      if (is_wait_for_receive) {
        if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUSES_IGNORE) ?
                          savedvals[i].comm->group()->rank(status[i].MPI_SOURCE) : savedvals[i].src;
        TRACE_smpi_recv(rank_traced, src_traced, dst_traced,savedvals[i].tag);
      }
    }
  }
  TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
  xbt_free(savedvals);

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = simgrid::smpi::Request::waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = simgrid::smpi::Request::testsome(incount, requests, indices, status);
    retval    = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_ARG;
  } else {
    int rank        = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced = comm->group()->index(root);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BCAST;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);
    if (comm->size() > 1)
      simgrid::smpi::Colls::bcast(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Barrier(MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BARRIER;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    simgrid::smpi::Colls::barrier(comm);

    //Barrier can be used to synchronize RMA calls. Finish all requests from comm before.
    comm->finish_rma_calls();

    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) || ((comm->rank() == root) && (recvcount <0))){
    retval = MPI_ERR_COUNT;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtmptype->size();
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    simgrid::smpi::Colls::gather(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }

    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced        = comm->group()->index(root);
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv = recvtype->size();
    if (comm->rank() == root) {
      extra->recvcounts = xbt_new(int, size);
      for (int i = 0; i < size; i++) // copy data to avoid bad free
        extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
    }
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) ||
            (recvcount <0)){
    retval = MPI_ERR_COUNT;
  } else {
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*recvcount*comm->rank();
      sendcount=recvcount;
      sendtype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHER;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    simgrid::smpi::Colls::allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) || (recvtype == MPI_DATATYPE_NULL)) {
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {

    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*displs[comm->rank()];
      sendcount=recvcounts[comm->rank()];
      sendtype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHERV;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv    = recvtype->size();
    extra->recvcounts = xbt_new(int, size);
    for (i                 = 0; i < size; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    simgrid::smpi::Colls::allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((comm->rank() == root) && (not sendtype->is_valid())) ||
             ((recvbuf != MPI_IN_PLACE) && (not recvtype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf == recvbuf) ||
      ((comm->rank()==root) && sendcount>0 && (sendbuf == nullptr))){
    retval = MPI_ERR_BUFFER;
  }else {

    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcount;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    simgrid::smpi::Colls::scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL)) ||
             ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL))) {
    retval = MPI_ERR_TYPE;
  } else {
    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcounts[comm->rank()];
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced        = comm->group()->index(root);
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send = sendtype->size();
    if (comm->rank() == root) {
      extra->sendcounts = xbt_new(int, size);
      for (int i = 0; i < size; i++) // copy data to avoid bad free
        extra->sendcounts[i] = sendcounts[i] * dt_size_send;
    }
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid() || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    extra->root      = root_traced;

    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    simgrid::smpi::Colls::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  int retval = 0;

  smpi_bench_end();
  if (not datatype->is_valid() || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    op->apply(inbuf, inoutbuf, &count, datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    if( sendbuf == MPI_IN_PLACE ) {
      sendtmpbuf = static_cast<char*>(xbt_malloc(count*datatype->get_extent()));
      simgrid::smpi::Datatype::copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLREDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    simgrid::smpi::Colls::allreduce(sendtmpbuf, recvbuf, count, datatype, op, comm);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::scan(sendbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_EXSCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::exscan(sendtmpbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = datatype->size();
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, size);
    int totalcount    = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_send;
      totalcount += recvcounts[i];
    }
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(totalcount * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, totalcount * datatype->size());
    }

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcount < 0) {
    retval = MPI_ERR_ARG;
  } else {
    int count = comm->size();

    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = count;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = datatype->size();
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, count);
    for (int i             = 0; i < count; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcount * dt_size_send;
    void* sendtmpbuf       = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * count * datatype->size());
    }

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    int* recvcounts = static_cast<int*>(xbt_malloc(count * sizeof(int)));
    for (int i      = 0; i < count; i++)
      recvcounts[i] = recvcount;
    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    xbt_free(recvcounts);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALL;

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int sendtmpcount         = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * comm->size() * recvtype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * comm->size() * recvtype->size());
      sendtmpcount = recvcount;
      sendtmptype  = recvtype;
    }

    int known        = 0;
    extra->datatype1 = encode_datatype(sendtmptype, &known);
    if (known == 0)
      extra->send_size = sendtmpcount * sendtmptype->size();
    else
      extra->send_size = sendtmpcount;
    extra->datatype2   = encode_datatype(recvtype, &known);
    if (known == 0)
      extra->recv_size = recvcount * recvtype->size();
    else
      extra->recv_size = recvcount;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::alltoall(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
             recvdisps == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process()->index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALLV;
    extra->send_size       = 0;
    extra->recv_size       = 0;
    extra->recvcounts      = xbt_new(int, size);
    extra->sendcounts      = xbt_new(int, size);
    int known              = 0;
    extra->datatype2       = encode_datatype(recvtype, &known);
    int dt_size_recv       = recvtype->size();

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int* sendtmpcounts       = sendcounts;
    int* sendtmpdisps        = senddisps;
    MPI_Datatype sendtmptype = sendtype;
    int maxsize              = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recv_size += recvcounts[i] * dt_size_recv;
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
      if (((recvdisps[i] + recvcounts[i]) * dt_size_recv) > maxsize)
        maxsize = (recvdisps[i] + recvcounts[i]) * dt_size_recv;
    }

    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(maxsize));
      memcpy(sendtmpbuf, recvbuf, maxsize);
      sendtmpcounts = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpcounts, recvcounts, size * sizeof(int));
      sendtmpdisps = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpdisps, recvdisps, size * sizeof(int));
      sendtmptype = recvtype;
    }

    extra->datatype1 = encode_datatype(sendtmptype, &known);
    int dt_size_send = 1;
    dt_size_send     = sendtmptype->size();

    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->send_size += sendtmpcounts[i] * dt_size_send;
      extra->sendcounts[i] = sendtmpcounts[i] * dt_size_send;
    }
    extra->num_processes = size;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);
    retval = simgrid::smpi::Colls::alltoallv(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
    }
  }

  smpi_bench_begin();
  return retval;
}


int PMPI_Get_processor_name(char *name, int *resultlen)
{
  strncpy(name, SIMIX_host_self()->cname(), strlen(SIMIX_host_self()->cname()) < MPI_MAX_PROCESSOR_NAME - 1
                                                ? strlen(SIMIX_host_self()->cname()) + 1
                                                : MPI_MAX_PROCESSOR_NAME - 1);
  *resultlen = strlen(name) > MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  return MPI_SUCCESS;
}

int PMPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
{
  if (status == nullptr || count == nullptr) {
    return MPI_ERR_ARG;
  } else if (not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else {
    size_t size = datatype->size();
    if (size == 0) {
      *count = 0;
      return MPI_SUCCESS;
    } else if (status->count % size != 0) {
      return MPI_UNDEFINED;
    } else {
      *count = simgrid::smpi::Status::get_count(status, datatype);
      return MPI_SUCCESS;
    }
  }
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_contiguous(count, old_type, 0, new_type);
  }
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  if (datatype == nullptr || *datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else {
    (*datatype)->commit();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_vector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_hvector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return MPI_Type_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed_block(int count, int blocklength, int* indices, MPI_Datatype old_type,
                                   MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=static_cast<int*>(xbt_malloc(blocklength*count*sizeof(int)));
    for (int i    = 0; i < count; i++)
      blocklens[i]=blocklength;
    int retval    = simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                              MPI_Datatype* new_type) {
  return PMPI_Type_hindexed(count, blocklens,indices,old_type,new_type);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint* indices, MPI_Datatype old_type,
                                    MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=(int*)xbt_malloc(blocklength*count*sizeof(int));
    for (int i     = 0; i < count; i++)
      blocklens[i] = blocklength;
    int retval     = simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type) {
  if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_struct(count, blocklens, indices, old_types, new_type);
  }
}

int PMPI_Type_create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                            MPI_Datatype* new_type) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, new_type);
}

int PMPI_Error_class(int errorcode, int* errorclass) {
  // assume smpi uses only standard mpi error codes
  *errorclass=errorcode;
  return MPI_SUCCESS;
}

int PMPI_Initialized(int* flag) {
   *flag=(smpi_process()!=nullptr && smpi_process()->initialized());
   return MPI_SUCCESS;
}

/* The topo part of MPI_COMM_WORLD should always be nullptr. When other topologies will be implemented, not only should we
 * check if the topology is nullptr, but we should check if it is the good topology type (so we have to add a
 *  MPIR_Topo_Type field, and replace the MPI_Topology field by an union)*/

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, int* dims, int* periodic, int reorder, MPI_Comm* comm_cart) {
  if (comm_old == MPI_COMM_NULL){
    return MPI_ERR_COMM;
  } else if (ndims < 0 || (ndims > 0 && (dims == nullptr || periodic == nullptr)) || comm_cart == nullptr) {
    return MPI_ERR_ARG;
  } else{
    simgrid::smpi::Topo_Cart* topo = new simgrid::smpi::Topo_Cart(comm_old, ndims, dims, periodic, reorder, comm_cart);
    if(*comm_cart==MPI_COMM_NULL)
      delete topo;
    return MPI_SUCCESS;
  }
}

int PMPI_Cart_rank(MPI_Comm comm, int* coords, int* rank) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (coords == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->rank(coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (source == nullptr || dest == nullptr || direction < 0 ) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->shift(direction, displ, source, dest);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (rank < 0 || rank >= comm->size()) {
    return MPI_ERR_RANK;
  }
  if (maxdims <= 0) {
    return MPI_ERR_ARG;
  }
  if(coords == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->coords(rank, maxdims, coords);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  if(comm == nullptr || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if(maxdims <= 0 || dims == nullptr || periods == nullptr || coords == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->get(maxdims, dims, periods, coords);
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  if (comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (ndims == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->dim_get(ndims);
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
  if(dims == nullptr) {
    return MPI_ERR_ARG;
  }
  if (ndims < 1 || nnodes < 1) {
    return MPI_ERR_DIMS;
  }
  return simgrid::smpi::Topo_Cart::Dims_create(nnodes, ndims, dims);
}

int PMPI_Cart_sub(MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (comm_new == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology cart = topo->sub(remain_dims, comm_new);
  if(*comm_new==MPI_COMM_NULL)
      delete cart;
  if(cart==nullptr)
    return  MPI_ERR_ARG;
  return MPI_SUCCESS;
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  if (oldtype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  }
  int blocks[3]         = {1, 1, 1};
  MPI_Aint disps[3]     = {lb, 0, lb + extent};
  MPI_Datatype types[3] = {MPI_LB, oldtype, MPI_UB};

  *newtype = new simgrid::smpi::Type_Struct(oldtype->size(), lb, lb + extent, DT_FLAG_DERIVED, 3, blocks, disps, types);

  (*newtype)->addflag(~DT_FLAG_COMMITED);
  return MPI_SUCCESS;
}

int PMPI_Win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else if ((base == nullptr && size != 0) || disp_unit <= 0 || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    *win = new simgrid::smpi::Win( base, size, disp_unit, info, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_allocate( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else if (disp_unit <= 0 || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    void* ptr = xbt_malloc(size);
    if(ptr==nullptr)
      return MPI_ERR_NO_MEM;
    *static_cast<void**>(base) = ptr;
    *win = new simgrid::smpi::Win( ptr, size, disp_unit, info, comm,1);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_create_dynamic( MPI_Info info, MPI_Comm comm, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else{
    *win = new simgrid::smpi::Win(info, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size){
  int retval = 0;
  smpi_bench_end();
  if(win == MPI_WIN_NULL){
    retval = MPI_ERR_WIN;
  } else if ((base == nullptr && size != 0) || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    retval = win->attach(base, size);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_detach(MPI_Win win,  void *base){
  int retval = 0;
  smpi_bench_end();
  if(win == MPI_WIN_NULL){
    retval = MPI_ERR_WIN;
  } else if (base == nullptr){
    retval= MPI_ERR_OTHER;
  }else{
    retval = win->detach(base);
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Win_free( MPI_Win* win){
  int retval = 0;
  smpi_bench_end();
  if (win == nullptr || *win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  }else{
    delete *win;
    retval=MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_set_name(MPI_Win  win, char * name)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    win->set_name(name);
    return MPI_SUCCESS;
  }
}

int PMPI_Win_get_name(MPI_Win  win, char * name, int* len)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_WIN;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    win->get_name(name, len);
    return MPI_SUCCESS;
  }
}

int PMPI_Win_get_info(MPI_Win  win, MPI_Info* info)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_WIN;
  } else {
    *info = win->info();
    return MPI_SUCCESS;
  }
}

int PMPI_Win_set_info(MPI_Win  win, MPI_Info info)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_TYPE;
  } else {
    win->set_info(info);
    return MPI_SUCCESS;
  }
}

int PMPI_Win_get_group(MPI_Win  win, MPI_Group * group){
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_WIN;
  }else {
    win->get_group(group);
    (*group)->ref();
    return MPI_SUCCESS;
  }
}

int PMPI_Win_fence( int assert,  MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
  int rank = smpi_process()->index();
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
  retval = win->fence(assert);
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Rget( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, request);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
            (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int dst_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, nullptr);
    TRACE_smpi_send(rank, rank, dst_traced, SMPI_RMA_TAG, origin_count*origin_datatype->size());

    retval = win->put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Rput( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
            (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int dst_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, nullptr);
    TRACE_smpi_send(rank, rank, dst_traced, SMPI_RMA_TAG, origin_count*origin_datatype->size());

    retval = win->put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, request);

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Raccumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((not origin_datatype->is_valid()) || (not target_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op, request);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Get_accumulate(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, 
int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, 
MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0 || result_count <0) ||
             (origin_addr==nullptr && origin_count > 0 && op != MPI_NO_OP) ||
             (result_addr==nullptr && result_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((origin_datatype != MPI_DATATYPE_NULL && not origin_datatype->is_valid()) ||
             (not target_datatype->is_valid()) || (not result_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->get_accumulate( origin_addr, origin_count, origin_datatype, result_addr, 
                                  result_count, result_datatype, target_rank, target_disp, 
                                  target_count, target_datatype, op);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Rget_accumulate(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, 
int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, 
MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0 || result_count <0) ||
             (origin_addr==nullptr && origin_count > 0 && op != MPI_NO_OP) ||
             (result_addr==nullptr && result_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((origin_datatype != MPI_DATATYPE_NULL && not origin_datatype->is_valid()) ||
             (not target_datatype->is_valid()) || (not result_datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->get_accumulate( origin_addr, origin_count, origin_datatype, result_addr, 
                                  result_count, result_datatype, target_rank, target_disp, 
                                  target_count, target_datatype, op, request);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Fetch_and_op(void *origin_addr, void *result_addr, MPI_Datatype dtype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win){
  return PMPI_Get_accumulate(origin_addr, origin_addr==nullptr?0:1, dtype, result_addr, 1, dtype, target_rank, target_disp, 1, dtype, op, win);
}

int PMPI_Compare_and_swap(void *origin_addr, void *compare_addr,
        void *result_addr, MPI_Datatype datatype, int target_rank,
        MPI_Aint target_disp, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (win->dynamic()==0 && target_disp <0){ 
    //in case of dynamic window, target_disp can be mistakenly seen as negative, as it is an address
    retval = MPI_ERR_ARG;
  } else if (origin_addr==nullptr || result_addr==nullptr || compare_addr==nullptr){
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process()->index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->compare_and_swap( origin_addr, compare_addr, result_addr, datatype, 
                                  target_rank, target_disp);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  } else {
    int rank = smpi_process()->index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
    retval = win->post(group,assert);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  } else {
    int rank = smpi_process()->index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
    retval = win->start(group,assert);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_complete(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int rank = smpi_process()->index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);

    retval = win->complete();

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_wait(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int rank = smpi_process()->index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);

    retval = win->wait();

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (lock_type != MPI_LOCK_EXCLUSIVE && 
             lock_type != MPI_LOCK_SHARED) {
    retval = MPI_ERR_LOCKTYPE;
  } else if (rank == MPI_PROC_NULL){ 
    retval = MPI_SUCCESS;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->lock(lock_type,rank,assert);
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_unlock(int rank, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (rank == MPI_PROC_NULL){ 
    retval = MPI_SUCCESS;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->unlock(rank);
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_lock_all(int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->lock_all(assert);
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_unlock_all(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->unlock_all();
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush(int rank, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (rank == MPI_PROC_NULL){ 
    retval = MPI_SUCCESS;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->flush(rank);
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_local(int rank, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (rank == MPI_PROC_NULL){ 
    retval = MPI_SUCCESS;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->flush_local(rank);
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_all(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->flush_all();
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_local_all(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int myrank = smpi_process()->index();
    TRACE_smpi_collective_in(myrank, -1, __FUNCTION__, nullptr);
    retval = win->flush_local_all();
    TRACE_smpi_collective_out(myrank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr){
  void *ptr = xbt_malloc(size);
  if(ptr==nullptr)
    return MPI_ERR_NO_MEM;
  else {
    *static_cast<void**>(baseptr) = ptr;
    return MPI_SUCCESS;
  }
}

int PMPI_Free_mem(void *baseptr){
  xbt_free(baseptr);
  return MPI_SUCCESS;
}

int PMPI_Type_set_name(MPI_Datatype  datatype, char * name)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    datatype->set_name(name);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    datatype->get_name(name, len);
    return MPI_SUCCESS;
  }
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype){
  return static_cast<MPI_Datatype>(simgrid::smpi::F2C::f2c(datatype));
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype){
  return datatype->c2f();
}

MPI_Group PMPI_Group_f2c(MPI_Fint group){
  return simgrid::smpi::Group::f2c(group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group){
  return group->c2f();
}

MPI_Request PMPI_Request_f2c(MPI_Fint request){
  return static_cast<MPI_Request>(simgrid::smpi::Request::f2c(request));
}

MPI_Fint PMPI_Request_c2f(MPI_Request request) {
  return request->c2f();
}

MPI_Win PMPI_Win_f2c(MPI_Fint win){
  return static_cast<MPI_Win>(simgrid::smpi::Win::f2c(win));
}

MPI_Fint PMPI_Win_c2f(MPI_Win win){
  return win->c2f();
}

MPI_Op PMPI_Op_f2c(MPI_Fint op){
  return static_cast<MPI_Op>(simgrid::smpi::Op::f2c(op));
}

MPI_Fint PMPI_Op_c2f(MPI_Op op){
  return op->c2f();
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm){
  return static_cast<MPI_Comm>(simgrid::smpi::Comm::f2c(comm));
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm){
  return comm->c2f();
}

MPI_Info PMPI_Info_f2c(MPI_Fint info){
  return static_cast<MPI_Info>(simgrid::smpi::Info::f2c(info));
}

MPI_Fint PMPI_Info_c2f(MPI_Info info){
  return info->c2f();
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  smpi_copy_fn _copy_fn={copy_fn,nullptr,nullptr};
  smpi_delete_fn _delete_fn={delete_fn,nullptr,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Comm>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Comm>(keyval);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
    return comm->attr_delete<simgrid::smpi::Comm>(keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
  static int one = 1;
  static int zero = 0;
  static int tag_ub = INT_MAX;
  static int last_used_code = MPI_ERR_LASTCODE;

  if (comm==MPI_COMM_NULL){
    *flag = 0;
    return MPI_ERR_COMM;
  }

  switch (keyval) {
  case MPI_HOST:
  case MPI_IO:
  case MPI_APPNUM:
    *flag = 1;
    *static_cast<int**>(attr_value) = &zero;
    return MPI_SUCCESS;
  case MPI_UNIVERSE_SIZE:
    *flag = 1;
    *static_cast<int**>(attr_value) = &smpi_universe_size;
    return MPI_SUCCESS;
  case MPI_LASTUSEDCODE:
    *flag = 1;
    *static_cast<int**>(attr_value) = &last_used_code;
    return MPI_SUCCESS;
  case MPI_TAG_UB:
    *flag=1;
    *static_cast<int**>(attr_value) = &tag_ub;
    return MPI_SUCCESS;
  case MPI_WTIME_IS_GLOBAL:
    *flag = 1;
    *static_cast<int**>(attr_value) = &one;
    return MPI_SUCCESS;
  default:
    return comm->attr_get<simgrid::smpi::Comm>(keyval, attr_value, flag);
  }
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void* attr_value) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
  return comm->attr_put<simgrid::smpi::Comm>(keyval, attr_value);
}

int PMPI_Comm_get_attr (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
  return PMPI_Attr_get(comm, comm_keyval, attribute_val,flag);
}

int PMPI_Comm_set_attr (MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return PMPI_Attr_put(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_delete_attr (MPI_Comm comm, int comm_keyval)
{
  return PMPI_Attr_delete(comm, comm_keyval);
}

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  return PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Comm_free_keyval(int* keyval) {
  return PMPI_Keyval_free(keyval);
}

int PMPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_get<simgrid::smpi::Datatype>(type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_put<simgrid::smpi::Datatype>(type_keyval, attribute_val);
}

int PMPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_delete<simgrid::smpi::Datatype>(type_keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  smpi_copy_fn _copy_fn={nullptr,copy_fn,nullptr};
  smpi_delete_fn _delete_fn={nullptr,delete_fn,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Datatype>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Type_free_keyval(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Datatype>(keyval);
}

int PMPI_Win_get_attr (MPI_Win win, int keyval, void *attribute_val, int* flag)
{
  static MPI_Aint size;
  static int disp_unit;
  if (win==MPI_WIN_NULL)
    return MPI_ERR_TYPE;
  else{
  switch (keyval) {
    case MPI_WIN_BASE :
      *static_cast<void**>(attribute_val)  = win->base();
      *flag = 1;
      return MPI_SUCCESS;
    case MPI_WIN_SIZE :
      size = win->size();
      *static_cast<MPI_Aint**>(attribute_val)  = &size;
      *flag = 1;
      return MPI_SUCCESS;
    case MPI_WIN_DISP_UNIT :
      disp_unit=win->disp_unit();
      *static_cast<int**>(attribute_val)  = &disp_unit;
      *flag = 1;
      return MPI_SUCCESS;
    default:
      return win->attr_get<simgrid::smpi::Win>(keyval, attribute_val, flag);
  }
}

}

int PMPI_Win_set_attr (MPI_Win win, int type_keyval, void *attribute_val)
{
  if (win==MPI_WIN_NULL)
    return MPI_ERR_TYPE;
  else
    return win->attr_put<simgrid::smpi::Win>(type_keyval, attribute_val);
}

int PMPI_Win_delete_attr (MPI_Win win, int type_keyval)
{
  if (win==MPI_WIN_NULL)
    return MPI_ERR_TYPE;
  else
    return win->attr_delete<simgrid::smpi::Win>(type_keyval);
}

int PMPI_Win_create_keyval(MPI_Win_copy_attr_function* copy_fn, MPI_Win_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  smpi_copy_fn _copy_fn={nullptr, nullptr, copy_fn};
  smpi_delete_fn _delete_fn={nullptr, nullptr, delete_fn};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Win>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Win_free_keyval(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Win>(keyval);
}

int PMPI_Info_create( MPI_Info *info){
  if (info == nullptr)
    return MPI_ERR_ARG;
  *info = new simgrid::smpi::Info();
  return MPI_SUCCESS;
}

int PMPI_Info_set( MPI_Info info, char *key, char *value){
  if (info == nullptr || key == nullptr || value == nullptr)
    return MPI_ERR_ARG;
  info->set(key, value);
  return MPI_SUCCESS;
}

int PMPI_Info_free( MPI_Info *info){
  if (info == nullptr || *info==nullptr)
    return MPI_ERR_ARG;
  simgrid::smpi::Info::unref(*info);
  *info=MPI_INFO_NULL;
  return MPI_SUCCESS;
}

int PMPI_Info_get(MPI_Info info,char *key,int valuelen, char *value, int *flag){
  *flag=false;
  if (info == nullptr || key == nullptr || valuelen <0)
    return MPI_ERR_ARG;
  if (value == nullptr)
    return MPI_ERR_INFO_VALUE;
  return info->get(key, valuelen, value, flag);
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo){
  if (info == nullptr || newinfo==nullptr)
    return MPI_ERR_ARG;
  *newinfo = new simgrid::smpi::Info(info);
  return MPI_SUCCESS;
}

int PMPI_Info_delete(MPI_Info info, char *key){
  if (info == nullptr || key==nullptr)
    return MPI_ERR_ARG;
  return info->remove(key);
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys){
  if (info == nullptr || nkeys==nullptr)
    return MPI_ERR_ARG;
  return info->get_nkeys(nkeys);
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key){
  if (info == nullptr || key==nullptr || n<0 || n> MPI_MAX_INFO_KEY)
    return MPI_ERR_ARG;
  return info->get_nthkey(n, key);
}

int PMPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag){
  *flag=false;
  if (info == nullptr || key == nullptr || valuelen==nullptr)
    return MPI_ERR_ARG;
  return info->get_valuelen(key, valuelen, flag);
}

int PMPI_Unpack(void* inbuf, int incount, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  if(incount<0 || outcount < 0 || inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if (not type->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return type->unpack(inbuf, incount, position, outbuf,outcount, comm);
}

int PMPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  if(incount<0 || outcount < 0|| inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if (not type->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return type->pack(inbuf, incount, outbuf,outcount,position, comm);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  if(incount<0)
    return MPI_ERR_ARG;
  if (not datatype->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;

  *size=incount*datatype->size();

  return MPI_SUCCESS;
}

} // extern "C"
