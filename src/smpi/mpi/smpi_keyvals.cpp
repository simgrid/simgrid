/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//#include "private.hpp"
#include "smpi_keyvals.hpp"

namespace simgrid{
namespace smpi{

std::unordered_map<int, void*>* Keyval::attributes(){
  return &attributes_;
};


template <> int Keyval::call_deleter<Comm>(Comm* obj, smpi_key_elem elem, int keyval, void * value, int* flag){
  if(elem->delete_fn.comm_delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn.comm_delete_fn(obj, keyval, value, flag);
    if(ret!=MPI_SUCCESS)
      return ret;
  }
  return MPI_SUCCESS;
}

template <> int Keyval::call_deleter<Win>(Win* obj, smpi_key_elem elem, int keyval, void * value, int* flag){
  if(elem->delete_fn.win_delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn.win_delete_fn(obj, keyval, value, flag);
    if(ret!=MPI_SUCCESS)
      return ret;
  }
  return MPI_SUCCESS;
}

template <> int Keyval::call_deleter<Datatype>(Datatype* obj, smpi_key_elem elem, int keyval, void * value, int* flag){
  if(elem->delete_fn.type_delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn.type_delete_fn(obj, keyval, value, flag);
    if(ret!=MPI_SUCCESS)
      return ret;
  }
  return MPI_SUCCESS;
}

}
}
