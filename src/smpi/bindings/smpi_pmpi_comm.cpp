/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <climits>

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_info.hpp"
#include "smpi_errhandler.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, rank)
  *rank = comm->rank();
  return MPI_SUCCESS;
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, size)
  *size = comm->size();
  return MPI_SUCCESS;
}

int PMPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  CHECK_NULL(3, MPI_ERR_ARG, len)
  comm->get_name(name, len);
  return MPI_SUCCESS;
}

int PMPI_Comm_set_name (MPI_Comm comm, const char* name)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  comm->set_name(name);
  return MPI_SUCCESS;
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, group)
  *group = comm->group();
  if (*group != MPI_COMM_WORLD->group() && *group != MPI_GROUP_NULL && *group != MPI_GROUP_EMPTY)
    (*group)->ref();
  return MPI_SUCCESS;
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  CHECK_COMM2(1, comm1)
  CHECK_COMM2(2, comm2)
  CHECK_NULL(3, MPI_ERR_ARG, result)
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

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, newcomm)
  return comm->dup(newcomm);
}

int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, newcomm)
  comm->dup_with_info(info, newcomm);
  return MPI_SUCCESS;
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  CHECK_COMM(1)
  CHECK_GROUP(2, group)
  CHECK_NULL(3, MPI_ERR_ARG, newcomm)
  if (group->rank(simgrid::s4u::this_actor::get_pid()) == MPI_UNDEFINED) {
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
  CHECK_NULL(1, MPI_ERR_ARG, comm)
  CHECK_COMM2(1, *comm)
  CHECK_MPI_NULL(1, MPI_COMM_WORLD, MPI_ERR_COMM, *comm)
  simgrid::smpi::Comm::destroy(*comm);
  *comm = MPI_COMM_NULL;
  return MPI_SUCCESS;
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
  /* TODO: wait until all communication in comm are done */
  CHECK_NULL(1, MPI_ERR_ARG, comm)
  CHECK_COMM2(1, *comm)
  simgrid::smpi::Comm::destroy(*comm);
  *comm = MPI_COMM_NULL;
  return MPI_SUCCESS;
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  CHECK_NULL(4, MPI_ERR_ARG, comm_out)
  CHECK_COMM2(1, comm)
  if( color != MPI_UNDEFINED)//we use a negative value for MPI_UNDEFINED
    CHECK_NEGATIVE(3, MPI_ERR_ARG, color)
  smpi_bench_end();
  *comm_out = comm->split(color, key);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm)
{
  CHECK_COMM(1)
  CHECK_NULL(5, MPI_ERR_ARG, newcomm)
  smpi_bench_end();
  *newcomm = comm->split_type(split_type, key, info);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int, MPI_Comm* comm_out)
{
  CHECK_COMM(1)
  CHECK_GROUP(2, group)
  CHECK_NULL(5, MPI_ERR_ARG, comm_out)
  smpi_bench_end();
  int retval = MPI_Comm_create(comm, group, comm_out);
  smpi_bench_begin();
  return retval;
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm){
  if(comm==-1)
    return MPI_COMM_NULL;
  return simgrid::smpi::Comm::f2c(comm);
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm){
  if(comm==MPI_COMM_NULL)
    return -1;
  return comm->c2f();
}

int PMPI_Comm_get_attr (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
  return PMPI_Attr_get(comm, comm_keyval, attribute_val,flag);
}

int PMPI_Comm_set_attr (MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return PMPI_Attr_put(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info* info)
{
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_ARG, info)
  *info = comm->info();
  return MPI_SUCCESS;
}

int PMPI_Comm_set_info(MPI_Comm  comm, MPI_Info info)
{
  CHECK_COMM(1)
  comm->set_info(info);
  return MPI_SUCCESS;
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

int PMPI_Comm_test_inter(MPI_Comm comm, int* flag){
  CHECK_COMM(1)

  if(flag == nullptr)
    return MPI_ERR_ARG;
  *flag=false;
  return MPI_SUCCESS;
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval) {
  CHECK_COMM(1)
  CHECK_MPI_NULL(2, MPI_KEYVAL_INVALID, MPI_ERR_KEYVAL, keyval)
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else
    return comm->attr_delete<simgrid::smpi::Comm>(keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
  static int one = 1;
  static int zero = 0;
  static int tag_ub = INT_MAX;
  static int last_used_code = MPI_ERR_LASTCODE;
  static int universe_size;

  CHECK_NULL(4, MPI_ERR_ARG, flag)
  *flag = 0;
  CHECK_COMM(1)
  CHECK_MPI_NULL(2, MPI_KEYVAL_INVALID, MPI_ERR_KEYVAL, keyval)

  switch (keyval) {
  case MPI_HOST:
  case MPI_IO:
  case MPI_APPNUM:
    *flag = 1;
    *static_cast<int**>(attr_value) = &zero;
    return MPI_SUCCESS;
  case MPI_UNIVERSE_SIZE:
    *flag = 1;
    universe_size                   = smpi_get_universe_size();
    *static_cast<int**>(attr_value) = &universe_size;
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
  CHECK_COMM(1)
  CHECK_MPI_NULL(2, MPI_KEYVAL_INVALID, MPI_ERR_KEYVAL, keyval)
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else
    return comm->attr_put<simgrid::smpi::Comm>(keyval, attr_value);
}

int PMPI_Errhandler_free(MPI_Errhandler* errhandler){
  CHECK_NULL(1, MPI_ERR_ARG, errhandler)
  CHECK_MPI_NULL(1, MPI_ERRHANDLER_NULL, MPI_ERR_ARG, *errhandler)
  simgrid::smpi::Errhandler::unref(*errhandler);
  *errhandler = MPI_ERRHANDLER_NULL;
  return MPI_SUCCESS;
}

int PMPI_Errhandler_create(MPI_Handler_function* function, MPI_Errhandler* errhandler){
  CHECK_NULL(2, MPI_ERR_ARG, errhandler)
  *errhandler=new simgrid::smpi::Errhandler(function);
  (*errhandler)->add_f();
  return MPI_SUCCESS;
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler* errhandler){
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_ARG, errhandler)
  *errhandler=comm->errhandler();
  return MPI_SUCCESS;
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler){
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_ARG, errhandler)
  comm->set_errhandler(errhandler);
  return MPI_SUCCESS;
}

int PMPI_Comm_call_errhandler(MPI_Comm comm,int errorcode){
  CHECK_COMM(1)
  MPI_Errhandler err = comm->errhandler();
  err->call(comm, errorcode);
  simgrid::smpi::Errhandler::unref(err);
  return MPI_SUCCESS;
}

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhan){
  if(errhan==-1)
    return MPI_ERRHANDLER_NULL;
  return simgrid::smpi::Errhandler::f2c(errhan);
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhan){
  if(errhan==MPI_ERRHANDLER_NULL)
    return -1;
  return errhan->c2f();
}

int PMPI_Comm_create_errhandler( MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler){
  return MPI_Errhandler_create(function, errhandler);
}
int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler* errhandler){
  return PMPI_Errhandler_get(comm, errhandler);
}
int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler){
  return PMPI_Errhandler_set(comm, errhandler);
}
