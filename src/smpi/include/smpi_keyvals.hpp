/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_KEYVALS_HPP_INCLUDED
#define SMPI_KEYVALS_HPP_INCLUDED

#include "smpi/smpi.h"

#include <unordered_map>

struct smpi_delete_fn {
  MPI_Comm_delete_attr_function          *comm_delete_fn;
  MPI_Type_delete_attr_function          *type_delete_fn;
  MPI_Win_delete_attr_function           *win_delete_fn;
  MPI_Comm_delete_attr_function_fort     *comm_delete_fn_fort;
  MPI_Type_delete_attr_function_fort     *type_delete_fn_fort;
  MPI_Win_delete_attr_function_fort      *win_delete_fn_fort;
};

struct smpi_copy_fn {
  MPI_Comm_copy_attr_function          *comm_copy_fn;
  MPI_Type_copy_attr_function          *type_copy_fn;
  MPI_Win_copy_attr_function           *win_copy_fn;
  MPI_Comm_copy_attr_function_fort     *comm_copy_fn_fort;
  MPI_Type_copy_attr_function_fort     *type_copy_fn_fort;
  MPI_Win_copy_attr_function_fort      *win_copy_fn_fort;
};

struct smpi_key_elem {
  smpi_copy_fn copy_fn;
  smpi_delete_fn delete_fn;
  void* extra_state;
  int refcount;
};

namespace simgrid{
namespace smpi{

class Keyval{
  private:
    std::unordered_map<int, void*> attributes_;
  protected:
    std::unordered_map<int, void*>& attributes() { return attributes_; }

  public:
// Each subclass should have two members, as we want to separate the ones for Win, Comm, and Datatypes :
//    static std::unordered_map<int, smpi_key_elem> keyvals_;
//    static int keyval_id_;
    template <typename T>
    static int keyval_create(const smpi_copy_fn& copy_fn, const smpi_delete_fn& delete_fn, int* keyval,
                             void* extra_state);
    template <typename T> static int keyval_free(int* keyval);
    template <typename T> int attr_delete(int keyval);
    template <typename T> int attr_get(int keyval, void* attr_value, int* flag);
    template <typename T> int attr_put(int keyval, void* attr_value);
    template <typename T>
    static int call_deleter(T* obj, const smpi_key_elem& elem, int keyval, void* value, int* flag);
    template <typename T> void cleanup_attr();
};

template <typename T>
int Keyval::keyval_create(const smpi_copy_fn& copy_fn, const smpi_delete_fn& delete_fn, int* keyval, void* extra_state)
{
  smpi_key_elem value;
  value.copy_fn     = copy_fn;
  value.delete_fn   = delete_fn;
  value.extra_state = extra_state;
  value.refcount    = 1;

  *keyval = T::keyval_id_;
  T::keyvals_.emplace(*keyval, std::move(value));
  T::keyval_id_++;
  return MPI_SUCCESS;
}

template <typename T> int Keyval::keyval_free(int* keyval){
  /* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it is in use in an attribute */
  auto elem_it = T::keyvals_.find(*keyval);
  if (elem_it == T::keyvals_.end())
    return MPI_ERR_ARG;

  smpi_key_elem& elem = elem_it->second;
  elem.refcount--;
  if (elem.refcount == 0)
    T::keyvals_.erase(elem_it);
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

template <typename T> int Keyval::attr_delete(int keyval){
  auto elem_it = T::keyvals_.find(keyval);
  if (elem_it == T::keyvals_.end())
    return MPI_ERR_ARG;

  smpi_key_elem& elem = elem_it->second;
  elem.refcount--;
  void * value = nullptr;
  int flag=0;
  if(this->attr_get<T>(keyval, &value, &flag)==MPI_SUCCESS){
    int ret = call_deleter<T>((T*)this, elem, keyval,value,&flag);
    if(ret!=MPI_SUCCESS)
        return ret;
  }
  if (attributes().empty())
    return MPI_ERR_ARG;
  attributes().erase(keyval);
  return MPI_SUCCESS;
}


template <typename T> int Keyval::attr_get(int keyval, void* attr_value, int* flag){
  auto elem_it = T::keyvals_.find(keyval);
  if (elem_it == T::keyvals_.end())
    return MPI_ERR_ARG;

  auto attr = attributes().find(keyval);
  if (attr != attributes().end()) {
    *static_cast<void**>(attr_value) = attr->second;
    *flag=1;
  } else {
    *flag=0;
  }
  return MPI_SUCCESS;
}

template <typename T> int Keyval::attr_put(int keyval, void* attr_value){
  auto elem_it = T::keyvals_.find(keyval);
  if (elem_it == T::keyvals_.end())
    return MPI_ERR_ARG;

  smpi_key_elem& elem = elem_it->second;
  int flag=0;
  auto p  = attributes().emplace(keyval, attr_value);
  if (p.second) {
    elem.refcount++;
  } else {
    int ret = call_deleter<T>((T*)this, elem, keyval,p.first->second,&flag);
    // overwrite previous value
    p.first->second = attr_value;
    if(ret!=MPI_SUCCESS)
      return ret;
  }
  return MPI_SUCCESS;
}

template <typename T> void Keyval::cleanup_attr(){
  int flag = 0;
  for (auto const& it : attributes()) {
    auto elem_it = T::keyvals_.find(it.first);
    if (elem_it != T::keyvals_.end()) {
      smpi_key_elem& elem = elem_it->second;
      call_deleter<T>((T*)this, elem, it.first, it.second, &flag);
    } else {
      // already deleted, not a problem
      flag = 0;
    }
  }
}

}
}

#endif
