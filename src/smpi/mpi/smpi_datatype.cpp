/* smpi_datatype.cpp -- MPI primitives to handle datatypes                  */
/* Copyright (c) 2009-2017. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/modelchecker.h"
#include "private.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "smpi_process.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_datatype, smpi, "Logging specific to SMPI (datatype)");

#define CREATE_MPI_DATATYPE(name, type)               \
  static simgrid::smpi::Datatype mpi_##name (         \
    (char*) # name,                                   \
    sizeof(type),   /* size */                        \
    0,              /* lb */                          \
    sizeof(type),   /* ub = lb + size */              \
    DT_FLAG_BASIC  /* flags */                        \
  );                                                  \
const MPI_Datatype name = &mpi_##name;

#define CREATE_MPI_DATATYPE_NULL(name)                \
  static simgrid::smpi::Datatype mpi_##name (         \
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

std::unordered_map<int, smpi_key_elem> Datatype::keyvals_;
int Datatype::keyval_id_=0;

Datatype::Datatype(int size,MPI_Aint lb, MPI_Aint ub, int flags) : name_(nullptr), size_(size), lb_(lb), ub_(ub), flags_(flags), refcount_(1){
#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&(refcount_), sizeof(refcount_));
#endif
}

//for predefined types, so in_use = 0.
Datatype::Datatype(char* name, int size,MPI_Aint lb, MPI_Aint ub, int flags) : name_(name), size_(size), lb_(lb), ub_(ub), flags_(flags), refcount_(0){
#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&(refcount_), sizeof(refcount_));
#endif
}

Datatype::Datatype(Datatype *datatype, int* ret) : name_(nullptr), lb_(datatype->lb_), ub_(datatype->ub_), flags_(datatype->flags_), refcount_(1)
{
  flags_ &= ~DT_FLAG_PREDEFINED;
  *ret = MPI_SUCCESS;
  if(datatype->name_)
    name_ = xbt_strdup(datatype->name_);

  if (not datatype->attributes()->empty()) {
    int flag;
    void* value_out;
    for(auto it = datatype->attributes()->begin(); it != datatype->attributes()->end(); it++){
      smpi_key_elem elem = keyvals_.at((*it).first);

      if (elem != nullptr && elem->copy_fn.type_copy_fn != MPI_NULL_COPY_FN) {
        *ret = elem->copy_fn.type_copy_fn(datatype, (*it).first, nullptr, (*it).second, &value_out, &flag);
        if (*ret != MPI_SUCCESS) {
          break;
        }
        if (flag){
          elem->refcount++;
          attributes()->insert({(*it).first, value_out});
        }
      }
    }
  }
}

Datatype::~Datatype(){
  xbt_assert(refcount_ >= 0);

  if(flags_ & DT_FLAG_PREDEFINED)
    return;

  //if still used, mark for deletion
  if(refcount_!=0){
      flags_ |=DT_FLAG_DESTROYED;
      return;
  }

  cleanup_attr<Datatype>();

  xbt_free(name_);
}


void Datatype::ref(){

  refcount_++;

#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&(refcount_), sizeof(refcount_));
#endif
}

void Datatype::unref(MPI_Datatype datatype)
{
  if (datatype->refcount_ > 0)
    datatype->refcount_--;

  if (datatype->refcount_ == 0 && not(datatype->flags_ & DT_FLAG_PREDEFINED))
    delete datatype;

#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&(datatype->refcount_), sizeof(datatype->refcount_));
#endif
}

void Datatype::commit()
{
  flags_ |= DT_FLAG_COMMITED;
}

bool Datatype::is_valid(){
  return (flags_ & DT_FLAG_COMMITED);
}

bool Datatype::is_basic()
{
  return (flags_ & DT_FLAG_BASIC);
}

bool Datatype::is_replayable()
{
  return ((this==MPI_BYTE)||(this==MPI_DOUBLE)||(this==MPI_INT)||
          (this==MPI_CHAR)||(this==MPI_SHORT)||(this==MPI_LONG)||(this==MPI_FLOAT));
}

size_t Datatype::size(){
  return size_;
}

int Datatype::flags(){
  return flags_;
}

int Datatype::refcount(){
  return refcount_;
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

int Datatype::pack(void* inbuf, int incount, void* outbuf, int outcount, int* position,MPI_Comm comm){
  if (outcount - *position < incount*static_cast<int>(size_))
    return MPI_ERR_BUFFER;
  Datatype::copy(inbuf, incount, this, static_cast<char*>(outbuf) + *position, outcount, MPI_CHAR);
  *position += incount * size_;
  return MPI_SUCCESS;
}

int Datatype::unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount,MPI_Comm comm){
  if (outcount*static_cast<int>(size_)> insize)
    return MPI_ERR_BUFFER;
  Datatype::copy(static_cast<char*>(inbuf) + *position, insize, MPI_CHAR, outbuf, outcount, this);
  *position += outcount * size_;
  return MPI_SUCCESS;
}


int Datatype::copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype){

// FIXME Handle the case of a partial shared malloc.

  if(smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP){
    smpi_switch_data_segment(smpi_process()->index());
  }
  /* First check if we really have something to do */
  if (recvcount > 0 && recvbuf != sendbuf) {
    sendcount *= sendtype->size();
    recvcount *= recvtype->size();
    int count = sendcount < recvcount ? sendcount : recvcount;

    if (not(sendtype->flags() & DT_FLAG_DERIVED) && not(recvtype->flags() & DT_FLAG_DERIVED)) {
      if (not smpi_process()->replaying())
        memcpy(recvbuf, sendbuf, count);
    } else if (not(sendtype->flags() & DT_FLAG_DERIVED)) {
      recvtype->unserialize(sendbuf, recvbuf, count / recvtype->size(), MPI_REPLACE);
    } else if (not(recvtype->flags() & DT_FLAG_DERIVED)) {
      sendtype->serialize(sendbuf, recvbuf, count / sendtype->size());
    }else{

      void * buf_tmp = xbt_malloc(count);

      sendtype->serialize( sendbuf, buf_tmp,count/sendtype->size());
      recvtype->unserialize( buf_tmp, recvbuf,count/recvtype->size(), MPI_REPLACE);

      xbt_free(buf_tmp);
    }
  }

  return sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
}

//Default serialization method : memcpy.
void Datatype::serialize( void* noncontiguous_buf, void *contiguous_buf, int count){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+lb_;
  memcpy(contiguous_buf_char, noncontiguous_buf_char, count*size_);

}

void Datatype::unserialize( void* contiguous_buf, void *noncontiguous_buf, int count, MPI_Op op){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+lb_;
  int n=count;
  if(op!=MPI_OP_NULL)
    op->apply( contiguous_buf_char, noncontiguous_buf_char, &n, this);
}

int Datatype::create_contiguous(int count, MPI_Datatype old_type, MPI_Aint lb, MPI_Datatype* new_type){
  if(old_type->flags_ & DT_FLAG_DERIVED){
    //handle this case as a hvector with stride equals to the extent of the datatype
    return create_hvector(count, 1, old_type->get_extent(), old_type, new_type);
  }
  if(count>0)
    *new_type = new Type_Contiguous(count * old_type->size(), lb, lb + count * old_type->size(),
                                   DT_FLAG_DERIVED, count, old_type);
  else
    *new_type = new Datatype(count * old_type->size(), lb, lb + count * old_type->size(),0);
  return MPI_SUCCESS;
}

int Datatype::create_vector(int count, int block_length, int stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (block_length<0)
    return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=old_type->lb();
    ub=((count-1)*stride+block_length-1)*old_type->get_extent()+old_type->ub();
  }
  if(old_type->flags() & DT_FLAG_DERIVED || stride != block_length){
    *new_type = new Type_Vector(count * (block_length) * old_type->size(), lb, ub,
                                   DT_FLAG_DERIVED, count, block_length, stride, old_type);
    retval=MPI_SUCCESS;
  }else{
    /* in this situation the data are contiguous thus it's not required to serialize and unserialize it*/
    *new_type = new Datatype(count * block_length * old_type->size(), 0, ((count -1) * stride + block_length)*
                         old_type->size(), DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
  }
  return retval;
}


int Datatype::create_hvector(int count, int block_length, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (block_length<0)
    return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=old_type->lb();
    ub=((count-1)*stride)+(block_length-1)*old_type->get_extent()+old_type->ub();
  }
  if(old_type->flags() & DT_FLAG_DERIVED || stride != block_length*old_type->get_extent()){
    *new_type = new Type_Hvector(count * (block_length) * old_type->size(), lb, ub,
                                   DT_FLAG_DERIVED, count, block_length, stride, old_type);
    retval=MPI_SUCCESS;
  }else{
    /* in this situation the data are contiguous thus it's not required to serialize and unserialize it*/
    *new_type = new Datatype(count * block_length * old_type->size(), 0, count * block_length * old_type->size(), DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
  }
  return retval;
}

int Datatype::create_indexed(int count, int* block_lengths, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type){
  int size = 0;
  bool contiguous=true;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0]*old_type->get_extent();
    ub=indices[0]*old_type->get_extent() + block_lengths[0]*old_type->ub();
  }

  for (int i = 0; i < count; i++) {
    if (block_lengths[i] < 0)
      return MPI_ERR_ARG;
    size += block_lengths[i];

    if(indices[i]*old_type->get_extent()+old_type->lb()<lb)
      lb = indices[i]*old_type->get_extent()+old_type->lb();
    if(indices[i]*old_type->get_extent()+block_lengths[i]*old_type->ub()>ub)
      ub = indices[i]*old_type->get_extent()+block_lengths[i]*old_type->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i] != indices[i+1]) )
      contiguous=false;
  }
  if(old_type->flags_ & DT_FLAG_DERIVED)
    contiguous=false;

  if (not contiguous) {
    *new_type = new Type_Indexed(size * old_type->size(),lb,ub,
                                 DT_FLAG_DERIVED|DT_FLAG_DATA, count, block_lengths, indices, old_type);
  }else{
    Datatype::create_contiguous(size, old_type, lb, new_type);
  }
  return MPI_SUCCESS;
}

int Datatype::create_hindexed(int count, int* block_lengths, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type){
  int size = 0;
  bool contiguous=true;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + old_type->lb();
    ub=indices[0] + block_lengths[0]*old_type->ub();
  }
  for (int i = 0; i < count; i++) {
    if (block_lengths[i] < 0)
      return MPI_ERR_ARG;
    size += block_lengths[i];

    if(indices[i]+old_type->lb()<lb)
      lb = indices[i]+old_type->lb();
    if(indices[i]+block_lengths[i]*old_type->ub()>ub)
      ub = indices[i]+block_lengths[i]*old_type->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i]*(static_cast<int>(old_type->size())) != indices[i+1]) )
      contiguous=false;
  }
  if (old_type->flags_ & DT_FLAG_DERIVED || lb!=0)
    contiguous=false;

  if (not contiguous) {
    *new_type = new Type_Hindexed(size * old_type->size(),lb,ub,
                                   DT_FLAG_DERIVED|DT_FLAG_DATA, count, block_lengths, indices, old_type);
  }else{
    Datatype::create_contiguous(size, old_type, lb, new_type);
  }
  return MPI_SUCCESS;
}

int Datatype::create_struct(int count, int* block_lengths, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type){
  size_t size = 0;
  bool contiguous=true;
  size = 0;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + old_types[0]->lb();
    ub=indices[0] + block_lengths[0]*old_types[0]->ub();
  }
  bool forced_lb=false;
  bool forced_ub=false;
  for (int i = 0; i < count; i++) {
    if (block_lengths[i]<0)
      return MPI_ERR_ARG;
    if (old_types[i]->flags_ & DT_FLAG_DERIVED)
      contiguous=false;

    size += block_lengths[i]*old_types[i]->size();
    if (old_types[i]==MPI_LB){
      lb=indices[i];
      forced_lb=true;
    }
    if (old_types[i]==MPI_UB){
      ub=indices[i];
      forced_ub=true;
    }

    if (not forced_lb && indices[i] + old_types[i]->lb() < lb)
      lb = indices[i];
    if (not forced_ub && indices[i] + block_lengths[i] * old_types[i]->ub() > ub)
      ub = indices[i]+block_lengths[i]*old_types[i]->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i]*static_cast<int>(old_types[i]->size()) != indices[i+1]) )
      contiguous=false;
  }
  if (not contiguous) {
    *new_type = new Type_Struct(size, lb,ub, DT_FLAG_DERIVED|DT_FLAG_DATA,
                                count, block_lengths, indices, old_types);
  }else{
    Datatype::create_contiguous(size, MPI_CHAR, lb, new_type);
  }
  return MPI_SUCCESS;
}

Datatype* Datatype::f2c(int id){
  return static_cast<Datatype*>(F2C::f2c(id));
}


}
}

