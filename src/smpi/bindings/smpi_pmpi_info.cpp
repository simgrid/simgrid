/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_info.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Info_create( MPI_Info *info){
  if (info == nullptr)
    return MPI_ERR_ARG;
  *info = new simgrid::smpi::Info();
  return MPI_SUCCESS;
}

int PMPI_Info_set( MPI_Info info, const char *key, const char *value){
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

int PMPI_Info_get(MPI_Info info, const char *key,int valuelen, char *value, int *flag){
  *flag=false;
  if (info == nullptr || valuelen <0)
    return MPI_ERR_ARG;
  if (key == nullptr)
    return MPI_ERR_INFO_KEY;
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

int PMPI_Info_delete(MPI_Info info, const char *key){
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

int PMPI_Info_get_valuelen( MPI_Info info, const char *key, int *valuelen, int *flag){
  *flag=false;
  if (info == nullptr)
    return MPI_ERR_ARG;
  if (key == nullptr)
    return MPI_ERR_INFO_KEY;
  if (valuelen == nullptr)
    return MPI_ERR_INFO_VALUE;
  return info->get_valuelen(key, valuelen, flag);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info){
  if(info==-1)
    return MPI_INFO_NULL;
  return static_cast<MPI_Info>(simgrid::smpi::Info::f2c(info));
}

MPI_Fint PMPI_Info_c2f(MPI_Info info){
  if(info==MPI_INFO_NULL)
    return -1;
  return info->c2f();
}
