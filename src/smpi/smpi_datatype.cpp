/* smpi_datatype.cpp -- MPI primitives to handle datatypes                      */
/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "private.h"
#include "simgrid/modelchecker.h"
#include "smpi_mpi_dt_private.h"
#include "xbt/replay.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <xbt/ex.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi_dt, smpi, "Logging specific to SMPI (datatype)");

xbt_dict_t smpi_type_keyvals = nullptr;
int type_keyval_id=0;//avoid collisions


#define CREATE_MPI_DATATYPE(name, type)               \
  static Datatype mpi_##name (         \
    (char*) # name,                                   \
    sizeof(type),   /* size */                        \
    0,              /* lb */                          \
    sizeof(type),   /* ub = lb + size */              \
    DT_FLAG_BASIC  /* flags */                       \
  );                                                  \
const MPI_Datatype name = &mpi_##name;

#define CREATE_MPI_DATATYPE_NULL(name)                \
  static Datatype mpi_##name (         \
    (char*) # name,                                   \
    0,              /* size */                        \
    0,              /* lb */                          \
    0,              /* ub = lb + size */              \
    DT_FLAG_BASIC  /* flags */                       \
  );                                                  \
const MPI_Datatype name = &mpi_##name;

// Predefined data types
CREATE_MPI_DATATYPE(MPI_CHAR, char);
CREATE_MPI_DATATYPE(MPI_SHORT, short);
CREATE_MPI_DATATYPE(MPI_INT, int);
CREATE_MPI_DATATYPE(MPI_LONG, long);
CREATE_MPI_DATATYPE(MPI_LONG_LONG, long long);
CREATE_MPI_DATATYPE(MPI_SIGNED_CHAR, signed char);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_CHAR, unsigned char);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_SHORT, unsigned short);
CREATE_MPI_DATATYPE(MPI_UNSIGNED, unsigned int);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_LONG, unsigned long);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_LONG_LONG, unsigned long long);
CREATE_MPI_DATATYPE(MPI_FLOAT, float);
CREATE_MPI_DATATYPE(MPI_DOUBLE, double);
CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE, long double);
CREATE_MPI_DATATYPE(MPI_WCHAR, wchar_t);
CREATE_MPI_DATATYPE(MPI_C_BOOL, bool);
CREATE_MPI_DATATYPE(MPI_BYTE, int8_t);
CREATE_MPI_DATATYPE(MPI_INT8_T, int8_t);
CREATE_MPI_DATATYPE(MPI_INT16_T, int16_t);
CREATE_MPI_DATATYPE(MPI_INT32_T, int32_t);
CREATE_MPI_DATATYPE(MPI_INT64_T, int64_t);
CREATE_MPI_DATATYPE(MPI_UINT8_T, uint8_t);
CREATE_MPI_DATATYPE(MPI_UINT16_T, uint16_t);
CREATE_MPI_DATATYPE(MPI_UINT32_T, uint32_t);
CREATE_MPI_DATATYPE(MPI_UINT64_T, uint64_t);
CREATE_MPI_DATATYPE(MPI_C_FLOAT_COMPLEX, float _Complex);
CREATE_MPI_DATATYPE(MPI_C_DOUBLE_COMPLEX, double _Complex);
CREATE_MPI_DATATYPE(MPI_C_LONG_DOUBLE_COMPLEX, long double _Complex);
CREATE_MPI_DATATYPE(MPI_AINT, MPI_Aint);
CREATE_MPI_DATATYPE(MPI_OFFSET, MPI_Offset);

CREATE_MPI_DATATYPE(MPI_FLOAT_INT, float_int);
CREATE_MPI_DATATYPE(MPI_LONG_INT, long_int);
CREATE_MPI_DATATYPE(MPI_DOUBLE_INT, double_int);
CREATE_MPI_DATATYPE(MPI_SHORT_INT, short_int);
CREATE_MPI_DATATYPE(MPI_2INT, int_int);
CREATE_MPI_DATATYPE(MPI_2FLOAT, float_float);
CREATE_MPI_DATATYPE(MPI_2DOUBLE, double_double);
CREATE_MPI_DATATYPE(MPI_2LONG, long_long);

CREATE_MPI_DATATYPE(MPI_REAL, float);
CREATE_MPI_DATATYPE(MPI_REAL4, float);
CREATE_MPI_DATATYPE(MPI_REAL8, float);
CREATE_MPI_DATATYPE(MPI_REAL16, double);
CREATE_MPI_DATATYPE_NULL(MPI_COMPLEX8);
CREATE_MPI_DATATYPE_NULL(MPI_COMPLEX16);
CREATE_MPI_DATATYPE_NULL(MPI_COMPLEX32);
CREATE_MPI_DATATYPE(MPI_INTEGER1, int);
CREATE_MPI_DATATYPE(MPI_INTEGER2, int16_t);
CREATE_MPI_DATATYPE(MPI_INTEGER4, int32_t);
CREATE_MPI_DATATYPE(MPI_INTEGER8, int64_t);
CREATE_MPI_DATATYPE(MPI_INTEGER16, integer128_t);

CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE_INT, long_double_int);

CREATE_MPI_DATATYPE_NULL(MPI_UB);
CREATE_MPI_DATATYPE_NULL(MPI_LB);
CREATE_MPI_DATATYPE(MPI_PACKED, char);
// Internal use only
CREATE_MPI_DATATYPE(MPI_PTR, void*);

namespace simgrid{
namespace smpi{

Datatype::Datatype(int size,int lb, int ub, int flags) : name_(nullptr), lb_(lb), ub_(ub), flags_(flags), attributes_(nullptr), in_use_(1){
#if HAVE_MC
  if(MC_is_active())
    MC_ignore(&(in_use_), sizeof(in_use_));
#endif
}

//for predefined types, so in_use = 0.
Datatype::Datatype(char* name, int size,int lb, int ub, int flags) : name_(name), lb_(lb), ub_(ub), flags_(flags), attributes_(nullptr), in_use_(0){
#if HAVE_MC
  if(MC_is_active())
    MC_ignore(&(in_use_), sizeof(in_use_));
#endif
}


//TODO : subtypes ?
Datatype::Datatype(Datatype &datatype) : lb_(datatype.lb_), ub_(datatype.ub_), flags_(datatype.flags_), in_use_(1)
{
  flags_ &= ~DT_FLAG_PREDEFINED;
  if(datatype.name_)
    name_ = xbt_strdup(datatype.name_);
  if(datatype.attributes_ !=nullptr){
    attributes_ = xbt_dict_new_homogeneous(nullptr);
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    int flag;
    void* value_in;
    void* value_out;
    xbt_dict_foreach (datatype.attributes_, cursor, key, value_in) {
      smpi_type_key_elem elem =
          static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, key, sizeof(int)));
      if (elem != nullptr && elem->copy_fn != MPI_NULL_COPY_FN) {
        int ret = elem->copy_fn(&datatype, atoi(key), nullptr, value_in, &value_out, &flag);
        if (ret != MPI_SUCCESS) {
//          smpi_datatype_unuse(*new_t);
//          *new_t = MPI_DATATYPE_NULL;
          xbt_dict_cursor_free(&cursor);
        }
        if (flag)
          xbt_dict_set_ext(attributes_, key, sizeof(int), value_out, nullptr);
      }
    }
  }
}

Datatype::~Datatype(){
  xbt_assert(in_use_ >= 0);

  if(flags_ & DT_FLAG_PREDEFINED)
    return;

  //if still used, mark for deletion
  if(in_use_!=0){
      flags_ |=DT_FLAG_DESTROYED;
      return;
  }

  if(attributes_ !=nullptr){
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    void * value;
    int flag;
    xbt_dict_foreach(attributes_, cursor, key, value){
      smpi_type_key_elem elem =
          static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, key, sizeof(int)));
      if(elem!=nullptr && elem->delete_fn!=nullptr)
        elem->delete_fn(this,*key, value, &flag);
    }
    xbt_dict_free(&attributes_);
  }

  xbt_free(name_);
}


void Datatype::use(){

  in_use_++;

#if HAVE_MC
  if(MC_is_active())
    MC_ignore(&(in_use_), sizeof(in_use_));
#endif
}

void Datatype::unuse()
{
  if (in_use_ > 0)
    in_use_--;

  if (in_use_ == 0)
    this->~Datatype();

#if HAVE_MC
  if(MC_is_active())
    MC_ignore(&(in_use_), sizeof(in_use_));
#endif
}

void Datatype::commit()
{
  flags_ |= DT_FLAG_COMMITED;
}


bool Datatype::is_valid(){
  return (flags_ & DT_FLAG_COMMITED);
}

size_t Datatype::size(){
  return size_;
}

int Datatype::flags(){
  return flags_;
}

void Datatype::addflag(int flag){
  flags_ &= flag;
}

MPI_Aint Datatype::lb(){
  return lb_;
}

MPI_Aint Datatype::ub(){
  return ub_;
}

char* Datatype::name(){
  return name_;
}


int Datatype::extent(MPI_Aint * lb, MPI_Aint * extent){
  *lb = lb_;
  *extent = ub_ - lb_;
  return MPI_SUCCESS;
}

MPI_Aint Datatype::get_extent(){
  return ub_ - lb_;
}

void Datatype::get_name(char* name, int* length){
  *length = strlen(name_);
  strncpy(name, name_, *length+1);
}

void Datatype::set_name(char* name){
  if(name_!=nullptr &&  (flags_ & DT_FLAG_PREDEFINED) == 0)
    xbt_free(name_);
  name_ = xbt_strdup(name);
}

int Datatype::attr_delete(int keyval){
  smpi_type_key_elem elem =
    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  if(elem->delete_fn!=MPI_NULL_DELETE_FN){
    void * value = nullptr;
    int flag;
    if(smpi_type_attr_get(this, keyval, &value, &flag)==MPI_SUCCESS){
      int ret = elem->delete_fn(this, keyval, value, &flag);
      if(ret!=MPI_SUCCESS) 
        return ret;
    }
  }  
  if(attributes_==nullptr)
    return MPI_ERR_ARG;

  xbt_dict_remove_ext(attributes_, reinterpret_cast<const char*>(&keyval), sizeof(int));
  return MPI_SUCCESS;
}


int Datatype::attr_get(int keyval, void* attr_value, int* flag){
  smpi_type_key_elem elem =
    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  if(attributes_==nullptr){
    *flag=0;
    return MPI_SUCCESS;
  }
  try {
    *static_cast<void**>(attr_value) = xbt_dict_get_ext(attributes_, reinterpret_cast<const char*>(&keyval), sizeof(int));
    *flag=1;
  }
  catch (xbt_ex& ex) {
    *flag=0;
  }
  return MPI_SUCCESS;
}

int Datatype::attr_put(int keyval, void* attr_value){
  if(smpi_type_keyvals==nullptr)
    smpi_type_keyvals = xbt_dict_new_homogeneous(nullptr);
  smpi_type_key_elem elem =
     static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  int flag;
  void* value = nullptr;
  smpi_type_attr_get(this, keyval, &value, &flag);
  if(flag!=0 && elem->delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn(this, keyval, value, &flag);
    if(ret!=MPI_SUCCESS) 
      return ret;
  }
  if(attributes_==nullptr)
    attributes_ = xbt_dict_new_homogeneous(nullptr);

  xbt_dict_set_ext(attributes_, reinterpret_cast<const char*>(&keyval), sizeof(int), attr_value, nullptr);
  return MPI_SUCCESS;
}

int Datatype::keyval_create(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval, void* extra_state){
  if(smpi_type_keyvals==nullptr)
    smpi_type_keyvals = xbt_dict_new_homogeneous(nullptr);

  smpi_type_key_elem value = (smpi_type_key_elem) xbt_new0(s_smpi_mpi_type_key_elem_t,1);

  value->copy_fn=copy_fn;
  value->delete_fn=delete_fn;

  *keyval = type_keyval_id;
  xbt_dict_set_ext(smpi_type_keyvals,reinterpret_cast<const char*>(keyval), sizeof(int),reinterpret_cast<void*>(value), nullptr);
  type_keyval_id++;
  return MPI_SUCCESS;
}

int Datatype::keyval_free(int* keyval){
  smpi_type_key_elem elem =
    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(keyval), sizeof(int)));
  if(elem==0){
    return MPI_ERR_ARG;
  }
  xbt_dict_remove_ext(smpi_type_keyvals, reinterpret_cast<const char*>(keyval), sizeof(int));
  xbt_free(elem);
  return MPI_SUCCESS;
}


int Datatype::pack(void* inbuf, int incount, void* outbuf, int outcount, int* position,MPI_Comm comm){
  if (outcount - *position < incount*static_cast<int>(size_))
    return MPI_ERR_BUFFER;
  Datatype::copy(inbuf, incount, this, static_cast<char*>(outbuf) + *position, outcount, MPI_CHAR);
  *position += incount * size_;
  return MPI_SUCCESS;
}

int Datatype::unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount,MPI_Comm comm){
  if (outcount*(int)size_> insize)
    return MPI_ERR_BUFFER;
  Datatype::copy(static_cast<char*>(inbuf) + *position, insize, MPI_CHAR, outbuf, outcount, this);
  *position += outcount * size_;
  return MPI_SUCCESS;
}


int Datatype::copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype){
//  int count;
  if(smpi_privatize_global_variables){
    smpi_switch_data_segment(smpi_process_index());
  }
  /* First check if we really have something to do */
  if (recvcount > 0 && recvbuf != sendbuf) {
    /* FIXME: treat packed cases */
    sendcount *= sendtype->size();
    recvcount *= recvtype->size();
//    count = sendcount < recvcount ? sendcount : recvcount;

//    if(sendtype->sizeof_substruct == 0 && recvtype->sizeof_substruct == 0) {
//      if(!smpi_process_get_replaying()) 
//        memcpy(recvbuf, sendbuf, count);
//    }
//    else if (sendtype->sizeof_substruct == 0)
//    {
//      s_smpi_subtype_t *subtype =  static_cast<s_smpi_subtype_t*>(recvtype->substruct);
//      subtype->unserialize( sendbuf, recvbuf, recvcount/smpi_datatype_size(recvtype), subtype, MPI_REPLACE);
//    }
//    else if (recvtype->sizeof_substruct == 0)
//    {
//      s_smpi_subtype_t *subtype =  static_cast<s_smpi_subtype_t*>(sendtype->substruct);
//      subtype->serialize(sendbuf, recvbuf, sendcount/smpi_datatype_size(sendtype), subtype);
//    }else{
//      s_smpi_subtype_t *subtype =  static_cast<s_smpi_subtype_t*>(sendtype->substruct);

//      void * buf_tmp = xbt_malloc(count);

//      subtype->serialize( sendbuf, buf_tmp,count/smpi_datatype_size(sendtype), subtype);
//      subtype =  static_cast<s_smpi_subtype_t*>(recvtype->substruct);
//      subtype->unserialize( buf_tmp, recvbuf,count/smpi_datatype_size(recvtype), subtype, MPI_REPLACE);

//      xbt_free(buf_tmp);
//    }
  }

  return sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
}

//Default serialization method : memcpy.
void Datatype::serialize( void* noncontiguous, void *contiguous, int count, void *type){
  char* contiguous_char = static_cast<char*>(contiguous);
  char* noncontiguous_char = static_cast<char*>(noncontiguous)+lb_;
  memcpy(contiguous_char, noncontiguous_char, count*size_);

}

void Datatype::unserialize( void* contiguous, void *noncontiguous, int count, void *type, MPI_Op op){
  char* contiguous_char = static_cast<char*>(contiguous);
  char* noncontiguous_char = static_cast<char*>(noncontiguous)+lb_;
  int n=count;
  if(op!=MPI_OP_NULL)
    op->apply( contiguous_char, noncontiguous_char, &n, this);
}



}
}

