/* Copyright (c) 2010, 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_KEYVALS_HPP_INCLUDED
#define SMPI_KEYVALS_HPP_INCLUDED

#include "private.h"
#include <unordered_map>
#include <xbt/ex.hpp>

typedef struct smpi_delete_fn{
  MPI_Comm_delete_attr_function          *comm_delete_fn;
  MPI_Type_delete_attr_function          *type_delete_fn;
  MPI_Win_delete_attr_function           *win_delete_fn;
} smpi_delete_fn;

typedef struct smpi_copy_fn{
  MPI_Comm_copy_attr_function          *comm_copy_fn;
  MPI_Type_copy_attr_function          *type_copy_fn;
  MPI_Win_copy_attr_function           *win_copy_fn;
} smpi_copy_fn;

typedef struct s_smpi_key_elem {
  smpi_copy_fn copy_fn;
  smpi_delete_fn delete_fn;
  int refcount;
} s_smpi_mpi_key_elem_t; 

typedef struct s_smpi_key_elem *smpi_key_elem;

namespace simgrid{
namespace smpi{

class Keyval{
  private:
    std::unordered_map<int, void*> attributes_;
  protected:
    std::unordered_map<int, void*>* attributes();
  public:
// Each subclass should have two members, as we want to separate the ones for Win, Comm, and Datatypes :  
//    static std::unordered_map<int, smpi_key_elem> keyvals_;
//    static int keyval_id_;
    template <typename T> static int keyval_create(smpi_copy_fn copy_fn, smpi_delete_fn delete_fn, int* keyval, void* extra_statee);
    template <typename T> static int keyval_free(int* keyval);
    template <typename T> int attr_delete(int keyval);
    template <typename T> int attr_get(int keyval, void* attr_value, int* flag);
    template <typename T> int attr_put(int keyval, void* attr_value);
    template <typename T> static int call_deleter(T* obj, smpi_key_elem elem, int keyval, void * value, int* flag);
    template <typename T> void cleanup_attr();
};

template <typename T> int Keyval::keyval_create(smpi_copy_fn copy_fn, smpi_delete_fn delete_fn, int* keyval, void* extra_state){

  smpi_key_elem value = (smpi_key_elem) xbt_new0(s_smpi_mpi_key_elem_t,1);

  value->copy_fn=copy_fn;
  value->delete_fn=delete_fn;
  value->refcount=1;

  *keyval = T::keyval_id_;
  T::keyvals_.insert({*keyval, value});
  T::keyval_id_++;
  return MPI_SUCCESS;
}

template <typename T> int Keyval::keyval_free(int* keyval){
/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
         * is in use in an attribute */
  smpi_key_elem elem = T::keyvals_.at(*keyval);
  if(elem==0){
    return MPI_ERR_ARG;
  }
  if(elem->refcount==1){
    T::keyvals_.erase(*keyval);
    xbt_free(elem);
  }else{
    elem->refcount--;
  }
  return MPI_SUCCESS;
}

template <typename T> int Keyval::attr_delete(int keyval){
  smpi_key_elem elem = T::keyvals_.at(keyval);
  if(elem==nullptr)
    return MPI_ERR_ARG;
  elem->refcount--;
  void * value = nullptr;
  int flag=0;
  if(this->attr_get<T>(keyval, &value, &flag)==MPI_SUCCESS){
    int ret = call_deleter<T>((T*)this, elem, keyval,value,&flag);
    if(ret!=MPI_SUCCESS)
        return ret;
  }
  if(attributes()->empty())
    return MPI_ERR_ARG;
  attributes()->erase(keyval);
  return MPI_SUCCESS;
}


template <typename T> int Keyval::attr_get(int keyval, void* attr_value, int* flag){
  smpi_key_elem elem = T::keyvals_.at(keyval);
  if(elem==nullptr)
    return MPI_ERR_ARG;
  if(attributes()->empty()){
    *flag=0;
    return MPI_SUCCESS;
  }
  try {
    *static_cast<void**>(attr_value) = attributes()->at(keyval);
    *flag=1;
  }
  catch (const std::out_of_range& oor) {
    *flag=0;
  }
  return MPI_SUCCESS;
}

template <typename T> int Keyval::attr_put(int keyval, void* attr_value){
  smpi_key_elem elem = T::keyvals_.at(keyval);
  if(elem==nullptr)
    return MPI_ERR_ARG;
  elem->refcount++;
  void * value = nullptr;
  int flag=0;
  this->attr_get<T>(keyval, &value, &flag);
  if(flag!=0){
    int ret = call_deleter<T>((T*)this, elem, keyval,value,&flag);
    if(ret!=MPI_SUCCESS)
        return ret;
  }
  attributes()->insert({keyval, attr_value});
  return MPI_SUCCESS;
}

template <typename T> void Keyval::cleanup_attr(){
  if(!attributes()->empty()){
    int flag=0;
    for(auto it : attributes_){
      try{
        smpi_key_elem elem = T::keyvals_.at(it.first);
        if(elem != nullptr){
          call_deleter<T>((T*)this, elem, it.first,it.second,&flag);
        }
      }catch(const std::out_of_range& oor) {
        //already deleted, not a problem;
        flag=0;
      }
    }
  }
}

}
}

#endif
