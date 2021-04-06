/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

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

#define CHECK_RMA\
  CHECK_BUFFER(1, origin_addr, origin_count)\
  CHECK_COUNT(2, origin_count)\
  CHECK_TYPE(3, origin_datatype)\
  CHECK_PROC_RMA(4, target_rank, win)\
  CHECK_COUNT(6, target_count)\
  CHECK_TYPE(7, target_datatype)

#define CHECK_TARGET_DISP(num)\
  if(win->dynamic()==0)\
    CHECK_NEGATIVE((num), MPI_ERR_RMA_RANGE, target_disp)

/* PMPI User level calls */

int PMPI_Win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){
  int retval = 0;
  CHECK_COMM(5)
  CHECK_NEGATIVE(2, MPI_ERR_OTHER, size)
  CHECK_NEGATIVE(3, MPI_ERR_OTHER, disp_unit)
  smpi_bench_end();
  if (base == nullptr && size != 0){
    retval= MPI_ERR_OTHER;
  }else{
    *win = new simgrid::smpi::Win( base, size, disp_unit, info, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_allocate( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win){
  CHECK_COMM(5)
  CHECK_NEGATIVE(2, MPI_ERR_OTHER, size)
  CHECK_NEGATIVE(3, MPI_ERR_OTHER, disp_unit)
  void* ptr = xbt_malloc(size);
  smpi_bench_end();
  *static_cast<void**>(base) = ptr;
  *win = new simgrid::smpi::Win( ptr, size, disp_unit, info, comm,1);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Win_allocate_shared( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win){
  CHECK_COMM(5)
  CHECK_NEGATIVE(2, MPI_ERR_OTHER, size)
  CHECK_NEGATIVE(3, MPI_ERR_OTHER, disp_unit)
  void* ptr = nullptr;
  int rank = comm->rank();
  if(rank==0){
     ptr = xbt_malloc(size*comm->size());
  }
  smpi_bench_end();
  simgrid::smpi::colls::bcast(&ptr, sizeof(void*), MPI_BYTE, 0, comm);
  simgrid::smpi::colls::barrier(comm);
  *static_cast<void**>(base) = (char*)ptr+rank*size;
  *win = new simgrid::smpi::Win( ptr, size, disp_unit, info, comm,rank==0);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Win_create_dynamic( MPI_Info info, MPI_Comm comm, MPI_Win *win){
  CHECK_COMM(2)
  smpi_bench_end();
  *win = new simgrid::smpi::Win(info, comm);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size){
  CHECK_WIN(1, win)
  CHECK_NEGATIVE(3, MPI_ERR_OTHER, size)
  if (base == nullptr && size != 0)
    return MPI_ERR_OTHER;
  smpi_bench_end();
  int retval = win->attach(base, size);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_detach(MPI_Win win, const void* base)
{
  CHECK_WIN(1, win)
  CHECK_NULL(2, MPI_ERR_OTHER, base)
  smpi_bench_end();
  int retval = win->detach(base);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_free( MPI_Win* win){
  CHECK_NULL(1, MPI_ERR_WIN, win)
  CHECK_WIN(1, (*win))
  smpi_bench_end();
  delete *win;
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Win_set_name(MPI_Win  win, const char * name)
{
  CHECK_WIN(1, win)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  win->set_name(name);
  return MPI_SUCCESS;
}

int PMPI_Win_get_name(MPI_Win  win, char * name, int* len)
{
  CHECK_WIN(1, win)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  win->get_name(name, len);
  return MPI_SUCCESS;
}

int PMPI_Win_get_info(MPI_Win  win, MPI_Info* info)
{
  CHECK_WIN(1, win)
  CHECK_NULL(2, MPI_ERR_ARG, info)
  *info = new simgrid::smpi::Info(win->info());
  return MPI_SUCCESS;
}

int PMPI_Win_set_info(MPI_Win  win, MPI_Info info)
{
  CHECK_WIN(1, win)
  win->set_info(info);
  return MPI_SUCCESS;
}

int PMPI_Win_get_group(MPI_Win  win, MPI_Group * group){
  CHECK_WIN(1, win)
  win->get_group(group);
  (*group)->ref();
  return MPI_SUCCESS;
}

int PMPI_Win_fence( int assert,  MPI_Win win){
  CHECK_WIN(2, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_fence"));
  int retval = win->fence(assert);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  CHECK_WIN(8, win)
  CHECK_RMA
  CHECK_TARGET_DISP(5)

  int retval = 0;
  smpi_bench_end();

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
 
  smpi_bench_begin();
  return retval;
}

int PMPI_Rget( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request){
  if(target_rank==MPI_PROC_NULL)
    *request = MPI_REQUEST_NULL;
  CHECK_WIN(8, win)
  CHECK_RMA
  CHECK_TARGET_DISP(5)
  CHECK_NULL(9, MPI_ERR_ARG, request)

  int retval = 0;
  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}

int PMPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  CHECK_WIN(8, win)
  CHECK_RMA
  CHECK_TARGET_DISP(5)

  int retval = 0;
  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}

int PMPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request){
  if(target_rank==MPI_PROC_NULL)
    *request = MPI_REQUEST_NULL;
  CHECK_WIN(8, win)
  CHECK_RMA
  CHECK_TARGET_DISP(5)
  CHECK_NULL(9, MPI_ERR_ARG, request)
  int retval = 0;
  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}

int PMPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  CHECK_WIN(9, win)
  CHECK_RMA
  CHECK_MPI_NULL(8, MPI_OP_NULL, MPI_ERR_OP, op)
  CHECK_TARGET_DISP(5)

  int retval = 0;

  smpi_bench_end();
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

  smpi_bench_begin();
  return retval;
}

int PMPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request){
  if(target_rank==MPI_PROC_NULL)
    *request = MPI_REQUEST_NULL;
  CHECK_WIN(9, win)
  CHECK_RMA
  CHECK_MPI_NULL(8, MPI_OP_NULL, MPI_ERR_OP, op)
  CHECK_TARGET_DISP(5)
  CHECK_NULL(10, MPI_ERR_ARG, request)

  int retval = 0;

  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}

int PMPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  if (op != MPI_NO_OP)
    CHECK_BUFFER(1, origin_addr, origin_count)
  CHECK_COUNT(2, origin_count)
  if(origin_count>0)
    CHECK_TYPE(3, origin_datatype)
  CHECK_BUFFER(4, result_addr, result_count)
  CHECK_COUNT(5, result_count)
  CHECK_TYPE(6, result_datatype)
  CHECK_WIN(12, win)
  CHECK_PROC_RMA(7, target_rank, win)
  CHECK_COUNT(9, target_count)
  CHECK_TYPE(10, target_datatype)
  CHECK_MPI_NULL(11, MPI_OP_NULL, MPI_ERR_OP, op)
  CHECK_TARGET_DISP(8)

  int retval = 0;
  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}


int PMPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request){
  if(target_rank==MPI_PROC_NULL)
    *request = MPI_REQUEST_NULL;
  CHECK_BUFFER(1, origin_addr, origin_count)
  CHECK_COUNT(2, origin_count)
  CHECK_TYPE(3, origin_datatype)
  CHECK_BUFFER(4, result_addr, result_count)
  CHECK_COUNT(5, result_count)
  CHECK_TYPE(6, result_datatype)
  CHECK_WIN(12, win)
  CHECK_PROC_RMA(7, target_rank, win)
  CHECK_COUNT(9, target_count)
  CHECK_TYPE(10, target_datatype)
  CHECK_MPI_NULL(11, MPI_OP_NULL, MPI_ERR_OP, op)
  CHECK_TARGET_DISP(8)
  CHECK_NULL(10, MPI_ERR_ARG, request)
  int retval = 0;
  smpi_bench_end();

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

  smpi_bench_begin();
  return retval;
}

int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype dtype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win){
  return PMPI_Get_accumulate(origin_addr, origin_addr==nullptr?0:1, dtype, result_addr, 1, dtype, target_rank, target_disp, 1, dtype, op, win);
}

int PMPI_Compare_and_swap(const void* origin_addr, void* compare_addr, void* result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win)
{
  CHECK_NULL(1, MPI_ERR_BUFFER, origin_addr)
  CHECK_NULL(2, MPI_ERR_BUFFER, compare_addr)
  CHECK_NULL(3, MPI_ERR_BUFFER, result_addr)
  CHECK_TYPE(4, datatype)
  CHECK_WIN(6, win)
  CHECK_PROC_RMA(5, target_rank, win)
  CHECK_TARGET_DISP(6)

  int retval = 0;

  smpi_bench_end();

  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  MPI_Group group;
  win->get_group(&group);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("Compare_and_swap", target_rank,
                                                     datatype->is_replayable() ? 1 : datatype->size(),
                                                     simgrid::smpi::Datatype::encode(datatype)));

  retval = win->compare_and_swap(origin_addr, compare_addr, result_addr, datatype, target_rank, target_disp);

  TRACE_smpi_comm_out(my_proc_id);

  smpi_bench_begin();
  return retval;
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win){
  CHECK_GROUP(1, group)
  CHECK_WIN(2, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_post"));
  int retval = win->post(group,assert);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win){
  CHECK_GROUP(1, group)
  CHECK_WIN(2, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_start"));
  int retval = win->start(group,assert);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_complete(MPI_Win win){
  CHECK_WIN(1, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_complete"));
  int retval = win->complete();
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_wait(MPI_Win win){
  CHECK_WIN(1, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_wait"));
  int retval = win->wait();
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win){
  CHECK_WIN(4, win)
  CHECK_PROC_RMA(2, rank, win)
  int retval = 0;
  smpi_bench_end();
  if (lock_type != MPI_LOCK_EXCLUSIVE &&
      lock_type != MPI_LOCK_SHARED) {
    retval = MPI_ERR_LOCKTYPE;
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
  CHECK_WIN(2, win)
  CHECK_PROC_RMA(1, rank, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_unlock"));
  int retval = win->unlock(rank);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_lock_all(int assert, MPI_Win win){
  CHECK_WIN(2, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_lock_all"));
  int retval = win->lock_all(assert);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_unlock_all(MPI_Win win){
  CHECK_WIN(1, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_unlock_all"));
  int retval = win->unlock_all();
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush(int rank, MPI_Win win){
  CHECK_WIN(2, win)
  CHECK_PROC_RMA(1, rank, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush"));
  int retval = win->flush(rank);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_local(int rank, MPI_Win win){
  CHECK_WIN(2, win)
  CHECK_PROC_RMA(1, rank, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_local"));
  int retval = win->flush_local(rank);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_all(MPI_Win win){
  CHECK_WIN(1, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_all"));
  int retval = win->flush_all();
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_flush_local_all(MPI_Win win){
  CHECK_WIN(1, win)
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Win_flush_local_all"));
  int retval = win->flush_local_all();
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_shared_query (MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr)
{
  CHECK_WIN(1, win)
  return win->shared_query(rank, size, disp_unit, baseptr);
}

int PMPI_Win_get_attr (MPI_Win win, int keyval, void *attribute_val, int* flag)
{
  static MPI_Aint size;
  static MPI_Aint disp_unit;
  CHECK_WIN(1, win)
  switch (keyval) {
    case MPI_WIN_BASE:
      *static_cast<void**>(attribute_val) = win->base();
      *flag                               = 1;
      return MPI_SUCCESS;
    case MPI_WIN_SIZE:
      size                                    = win->size();
      *static_cast<MPI_Aint**>(attribute_val) = &size;
      *flag                                   = 1;
      return MPI_SUCCESS;
    case MPI_WIN_DISP_UNIT:
      disp_unit                               = win->disp_unit();
      *static_cast<MPI_Aint**>(attribute_val) = &disp_unit;
      *flag                                   = 1;
      return MPI_SUCCESS;
    default:
     return win->attr_get<simgrid::smpi::Win>(keyval, attribute_val, flag);
  }
}

int PMPI_Win_set_attr (MPI_Win win, int type_keyval, void *attribute_val)
{
  CHECK_WIN(1, win)
  return win->attr_put<simgrid::smpi::Win>(type_keyval, attribute_val);
}

int PMPI_Win_delete_attr (MPI_Win win, int type_keyval)
{
  CHECK_WIN(1, win)
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
  return simgrid::smpi::Win::f2c(win);
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
  CHECK_WIN(1, win)
  if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  *errhandler=win->errhandler();
  return MPI_SUCCESS;
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler){
  CHECK_WIN(1, win)
  if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  win->set_errhandler(errhandler);
  return MPI_SUCCESS;
}

int PMPI_Win_call_errhandler(MPI_Win win,int errorcode){
  CHECK_WIN(1, win)
  MPI_Errhandler err = win->errhandler();
  err->call(win, errorcode);
  simgrid::smpi::Errhandler::unref(err);
  return MPI_SUCCESS;
}
