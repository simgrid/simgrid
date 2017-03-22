/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include <vector>
#include <xbt/ex.hpp>

namespace simgrid{
namespace smpi{

Info::Info():refcount_(1){
  dict_= xbt_dict_new_homogeneous(xbt_free_f);
}

Info::Info(Info* info):refcount_(1){
  dict_= xbt_dict_new_homogeneous(xbt_free_f);
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  void* data;
  xbt_dict_foreach(info->dict_,cursor,key,data){
    xbt_dict_set(dict_, key, xbt_strdup(static_cast<char*>(data)), nullptr);
  }
}

Info::~Info(){
  xbt_dict_free(&dict_);
}

void Info::ref(){
  refcount_++;
}

void Info::unref(Info* info){
  info->refcount_--;
  if(info->refcount_==0){
    delete info;
  }
}

void Info::set(char *key, char *value){
  xbt_dict_set(dict_, key, xbt_strdup(value), nullptr);
}



int Info::get(char *key, int valuelen, char *value, int *flag){
  *flag=false;
  char* tmpvalue=static_cast<char*>(xbt_dict_get_or_null(dict_, key));
  if(tmpvalue){
    memset(value, 0, valuelen);
    memcpy(value,tmpvalue, (strlen(tmpvalue) + 1 < static_cast<size_t>(valuelen)) ? strlen(tmpvalue) + 1 : valuelen);
    *flag=true;
  }
  return MPI_SUCCESS;
}


int Info::remove(char *key){
  try {
    xbt_dict_remove(dict_, key);
  }
  catch(xbt_ex& e){
    return MPI_ERR_INFO_NOKEY;
  }
  return MPI_SUCCESS;
}

int Info::get_nkeys(int *nkeys){
  *nkeys=xbt_dict_size(dict_);
  return MPI_SUCCESS;
}

int Info::get_nthkey(int n, char *key){
  xbt_dict_cursor_t cursor = nullptr;
  char *keyn;
  void* data;
  int num=0;
  xbt_dict_foreach(dict_,cursor,keyn,data){
    if(num==n){
      strncpy(key,keyn,strlen(keyn)+1);
      xbt_dict_cursor_free(&cursor);
      return MPI_SUCCESS;
    }
    num++;
  }
  return MPI_ERR_ARG;
}

int Info::get_valuelen(char *key, int *valuelen, int *flag){
  *flag=false;
  char* tmpvalue=static_cast<char*>(xbt_dict_get_or_null(dict_, key));
  if(tmpvalue){
    *valuelen=strlen(tmpvalue);
    *flag=true;
  }
  return MPI_SUCCESS;
}

Info* Info::f2c(int id){
  return static_cast<Info*>(F2C::f2c(id));
}

}
}

