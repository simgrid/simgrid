/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "smpi_win.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

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

int PMPI_Win_allocate_shared( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else if (disp_unit <= 0 || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    void* ptr = nullptr;
    int rank = comm->rank();
    if(rank==0){
       ptr = xbt_malloc(size*comm->size());
       if(ptr==nullptr)
         return MPI_ERR_NO_MEM;
    }
    
    simgrid::smpi::Colls::bcast(&ptr, sizeof(void*), MPI_BYTE, 0, comm);
    simgrid::smpi::Colls::barrier(comm);
    
    *static_cast<void**>(base) = (char*)ptr+rank*size;
    *win = new simgrid::smpi::Win( ptr, size, disp_unit, info, comm,rank==0);
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

int PMPI_Win_detach(MPI_Win win, const void* base)
{
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

int PMPI_Win_set_name(MPI_Win  win, const char * name)
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_fence"));
    retval = win->fence(assert);
    TRACE_smpi_comm_out(my_proc_id);
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData("Get", target_rank, origin_datatype->is_replayable()
                                                                               ? origin_count
                                                                               : origin_count * origin_datatype->size(),
                                                       simgrid::smpi::Datatype::encode(origin_datatype)));

    retval = win->get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_comm_out(my_proc_id);
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Rget", target_rank,
                           origin_datatype->is_replayable() ? origin_count : origin_count * origin_datatype->size(),
                           simgrid::smpi::Datatype::encode(origin_datatype)));

    retval = win->get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, request);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    int dst_traced = group->actor(target_rank)->get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData("Put", target_rank, origin_datatype->is_replayable()
                                                                               ? origin_count
                                                                               : origin_count * origin_datatype->size(),
                                                       simgrid::smpi::Datatype::encode(origin_datatype)));
    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, SMPI_RMA_TAG, origin_count * origin_datatype->size());

    retval = win->put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    int dst_traced = group->actor(target_rank)->get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Rput", target_rank,
                           origin_datatype->is_replayable() ? origin_count : origin_count * origin_datatype->size(),
                           simgrid::smpi::Datatype::encode(origin_datatype)));
    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, SMPI_RMA_TAG, origin_count * origin_datatype->size());

    retval = win->put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, request);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Accumulate", target_rank,
                           origin_datatype->is_replayable() ? origin_count : origin_count * origin_datatype->size(),
                           simgrid::smpi::Datatype::encode(origin_datatype)));
    retval = win->accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
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
  } else if (((origin_datatype == MPI_DATATYPE_NULL) || (target_datatype == MPI_DATATYPE_NULL)) ||
            ((not origin_datatype->is_valid()) || (not target_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Raccumulate", target_rank,
                           origin_datatype->is_replayable() ? origin_count : origin_count * origin_datatype->size(),
                           simgrid::smpi::Datatype::encode(origin_datatype)));

    retval = win->accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op, request);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
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
  } else if (((target_datatype == MPI_DATATYPE_NULL) || (result_datatype == MPI_DATATYPE_NULL)) ||
            (((origin_datatype != MPI_DATATYPE_NULL) && (not origin_datatype->is_valid())) || (not target_datatype->is_valid()) || (not result_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Get_accumulate", target_rank,
                           target_datatype->is_replayable() ? target_count : target_count * target_datatype->size(),
                           simgrid::smpi::Datatype::encode(target_datatype)));

    retval = win->get_accumulate( origin_addr, origin_count, origin_datatype, result_addr,
                                  result_count, result_datatype, target_rank, target_disp,
                                  target_count, target_datatype, op);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
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
  } else if (((target_datatype == MPI_DATATYPE_NULL) || (result_datatype == MPI_DATATYPE_NULL)) ||
            (((origin_datatype != MPI_DATATYPE_NULL) && (not origin_datatype->is_valid())) || (not target_datatype->is_valid()) || (not result_datatype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if(request == nullptr){
    retval = MPI_ERR_REQUEST;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData(
                           "Rget_accumulate", target_rank,
                           target_datatype->is_replayable() ? target_count : target_count * target_datatype->size(),
                           simgrid::smpi::Datatype::encode(target_datatype)));

    retval = win->get_accumulate( origin_addr, origin_count, origin_datatype, result_addr,
                                  result_count, result_datatype, target_rank, target_disp,
                                  target_count, target_datatype, op, request);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype dtype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win){
  return PMPI_Get_accumulate(origin_addr, origin_addr==nullptr?0:1, dtype, result_addr, 1, dtype, target_rank, target_disp, 1, dtype, op, win);
}

int PMPI_Compare_and_swap(const void* origin_addr, void* compare_addr, void* result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win)
{
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
  } else if ((datatype == MPI_DATATYPE_NULL) || (not datatype->is_valid())) {
    retval = MPI_ERR_TYPE;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    MPI_Group group;
    win->get_group(&group);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData("Compare_and_swap", target_rank,
                                                       datatype->is_replayable() ? 1 : datatype->size(),
                                                       simgrid::smpi::Datatype::encode(datatype)));

    retval = win->compare_and_swap(origin_addr, compare_addr, result_addr, datatype, target_rank, target_disp);

    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_post"));
    retval = win->post(group,assert);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_start"));
    retval = win->start(group,assert);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_complete"));

    retval = win->complete();

    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_wait"));

    retval = win->wait();

    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_lock"));
    retval = win->lock(lock_type,rank,assert);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_unlock"));
    retval = win->unlock(rank);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_lock_all"));
    retval = win->lock_all(assert);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_unlock_all"));
    retval = win->unlock_all();
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush"));
    retval = win->flush(rank);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_local"));
    retval = win->flush_local(rank);
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_all"));
    retval = win->flush_all();
    TRACE_smpi_comm_out(my_proc_id);
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
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_local_all"));
    retval = win->flush_local_all();
    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_shared_query (MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr)
{
  if (win==MPI_WIN_NULL)
    return MPI_ERR_TYPE;
  else
    return win->shared_query(rank, size, disp_unit, baseptr);
}

int PMPI_Win_get_attr (MPI_Win win, int keyval, void *attribute_val, int* flag)
{
  static MPI_Aint size;
  static MPI_Aint disp_unit;
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
      *static_cast<MPI_Aint**>(attribute_val)  = &disp_unit;
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
  smpi_copy_fn _copy_fn={nullptr, nullptr,copy_fn,nullptr, nullptr,nullptr};
  smpi_delete_fn _delete_fn={nullptr, nullptr,delete_fn,nullptr, nullptr,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Win>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Win_free_keyval(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Win>(keyval);
}

MPI_Win PMPI_Win_f2c(MPI_Fint win){
  if(win==-1)
    return MPI_WIN_NULL;
  return static_cast<MPI_Win>(simgrid::smpi::Win::f2c(win));
}

MPI_Fint PMPI_Win_c2f(MPI_Win win){
  if(win==MPI_WIN_NULL)
    return -1;
  return win->c2f();
}

int PMPI_Win_create_errhandler(MPI_Win_errhandler_function* function, MPI_Errhandler* errhandler){
  *errhandler=new simgrid::smpi::Errhandler(function);
  return MPI_SUCCESS;
}

int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler* errhandler){
  if (win == nullptr) {
    return MPI_ERR_WIN;
  } else if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  *errhandler=win->errhandler();
  return MPI_SUCCESS;
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler){
  if (win == nullptr) {
    return MPI_ERR_WIN;
  } else if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  win->set_errhandler(errhandler);
  return MPI_SUCCESS;
}

int PMPI_Win_call_errhandler(MPI_Win win,int errorcode){
  if (win == nullptr) {
    return MPI_ERR_WIN;
  }
  win->errhandler()->call(win, errorcode);
  return MPI_SUCCESS;
}
