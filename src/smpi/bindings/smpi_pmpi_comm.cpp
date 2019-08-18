/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

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

int PMPI_Comm_set_name (MPI_Comm comm, const char* name)
{
  if (comm == MPI_COMM_NULL)  {
    return MPI_ERR_COMM;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    comm->set_name(name);
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

int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (newcomm == nullptr) {
    return MPI_ERR_ARG;
  } else {
    comm->dup_with_info(info, newcomm);
    return MPI_SUCCESS;
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
  } else if (group->rank(simgrid::s4u::this_actor::get_pid()) == MPI_UNDEFINED) {
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

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm)
{
  int retval = 0;
  smpi_bench_end();

  if (newcomm == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *newcomm = comm->split_type(split_type, key, info);
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

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm){
  if(comm==-1)
    return MPI_COMM_NULL;
  return static_cast<MPI_Comm>(simgrid::smpi::Comm::f2c(comm));
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
  if (comm == MPI_COMM_NULL)  {
    return MPI_ERR_WIN;
  } else {
    *info = comm->info();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_set_info(MPI_Comm  comm, MPI_Info info)
{
  if (comm == MPI_COMM_NULL)  {
    return MPI_ERR_TYPE;
  } else {
    comm->set_info(info);
    return MPI_SUCCESS;
  }
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
  static int universe_size;

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
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
  return comm->attr_put<simgrid::smpi::Comm>(keyval, attr_value);
}

int PMPI_Errhandler_free(MPI_Errhandler* errhandler){
  if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Errhandler::unref(*errhandler);
  return MPI_SUCCESS;
}

int PMPI_Errhandler_create(MPI_Handler_function* function, MPI_Errhandler* errhandler){
  *errhandler=new simgrid::smpi::Errhandler(function);
  return MPI_SUCCESS;
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler* errhandler){
  if (comm == nullptr) {
    return MPI_ERR_COMM;
  } else if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  *errhandler=comm->errhandler();
  return MPI_SUCCESS;
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler){
  if (comm == nullptr) {
    return MPI_ERR_COMM;
  } else if (errhandler==nullptr){
    return MPI_ERR_ARG;
  }
  comm->set_errhandler(errhandler);
  return MPI_SUCCESS;
}

int PMPI_Comm_call_errhandler(MPI_Comm comm,int errorcode){
  if (comm == nullptr) {
    return MPI_ERR_COMM;
  }
  comm->errhandler()->call(comm, errorcode);
  return MPI_SUCCESS;
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
