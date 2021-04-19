/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_info.hpp"
#include "smpi_comm.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Info_create( MPI_Info *info){
  CHECK_NULL(1, MPI_ERR_ARG, info)
  *info = new simgrid::smpi::Info();
  return MPI_SUCCESS;
}

int PMPI_Info_set( MPI_Info info, const char *key, const char *value){
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_INFO_KEY, key)
  CHECK_NULL(3, MPI_ERR_INFO_VALUE, value)
  info->set(key, value);
  return MPI_SUCCESS;
}

int PMPI_Info_free( MPI_Info *info){
  CHECK_NULL(1, MPI_ERR_ARG, info)
  CHECK_INFO(1, *info)
  (*info)->mark_as_deleted();
  simgrid::smpi::Info::unref(*info);
  *info=MPI_INFO_NULL;
  return MPI_SUCCESS;
}

int PMPI_Info_get(MPI_Info info, const char *key,int valuelen, char *value, int *flag){
  CHECK_INFO(1, info)
  if (valuelen <0)
    return MPI_ERR_ARG;
  CHECK_NULL(2, MPI_ERR_INFO_KEY, key)
  CHECK_NULL(3, MPI_ERR_INFO_VALUE, value)
  CHECK_NULL(4, MPI_ERR_ARG, flag)
  *flag=false;
  return info->get(key, valuelen, value, flag);
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo){
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_ARG, newinfo)
  *newinfo = new simgrid::smpi::Info(info);
  return MPI_SUCCESS;
}

int PMPI_Info_delete(MPI_Info info, const char *key){
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_INFO_KEY, key)
  return info->remove(key);
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys){
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_ARG, nkeys)
  return info->get_nkeys(nkeys);
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key){
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_INFO_KEY, key)
  if (n<0 || n> MPI_MAX_INFO_KEY)
    return MPI_ERR_ARG;
  return info->get_nthkey(n, key);
}

int PMPI_Info_get_valuelen( MPI_Info info, const char *key, int *valuelen, int *flag){
  *flag=false;
  CHECK_INFO(1, info)
  CHECK_NULL(2, MPI_ERR_INFO_KEY, key)
  CHECK_NULL(2, MPI_ERR_INFO_VALUE, valuelen)
  return info->get_valuelen(key, valuelen, flag);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info){
  if(info==-1)
    return MPI_INFO_NULL;
  return simgrid::smpi::Info::f2c(info);
}

MPI_Fint PMPI_Info_c2f(MPI_Info info){
  if(info==MPI_INFO_NULL)
    return -1;
  return info->c2f();
}
