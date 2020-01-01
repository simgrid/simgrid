/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_info.hpp"
#include "simgrid/Exception.hpp"

namespace simgrid {
namespace smpi {

void Info::ref()
{
  refcount_++;
}

void Info::unref(Info* info){
  info->refcount_--;
  if(info->refcount_==0){
    delete info;
  }
}

int Info::get(const char *key, int valuelen, char *value, int *flag){
  *flag=false;
  auto val = map_.find(key);
  if (val != map_.end()) {
    std::string tmpvalue = val->second;

    memset(value, 0, valuelen);
    memcpy(value, tmpvalue.c_str(),
           (tmpvalue.length() + 1 < static_cast<size_t>(valuelen)) ? tmpvalue.length() + 1 : valuelen);
    *flag=true;
  }
  return MPI_SUCCESS;
}

int Info::remove(const char *key){
  if (map_.erase(key) == 0)
    return MPI_ERR_INFO_NOKEY;
  else
    return MPI_SUCCESS;
}

int Info::get_nkeys(int *nkeys){
  *nkeys = map_.size();
  return MPI_SUCCESS;
}

int Info::get_nthkey(int n, char *key){
  int num=0;
  for (auto const& elm : map_) {
    if (num == n) {
      strncpy(key, elm.first.c_str(), elm.first.length() + 1);
      return MPI_SUCCESS;
    }
    num++;
  }
  return MPI_ERR_ARG;
}

int Info::get_valuelen(const char *key, int *valuelen, int *flag){
  *flag=false;
  auto val = map_.find(key);
  if (val != map_.end()) {
    *valuelen = val->second.length();
    *flag=true;
  }
  return MPI_SUCCESS;
}

Info* Info::f2c(int id){
  return static_cast<Info*>(F2C::f2c(id));
}

}
}
