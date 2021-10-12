/* smpi_datatype.cpp -- MPI primitives to handle datatypes                  */
/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include <xbt/log.h>

#include <array>
#include <cstring>

namespace simgrid{
namespace smpi{

Datatype_contents::Datatype_contents(int combiner, int number_of_integers, const int* integers, int number_of_addresses,
                                     const MPI_Aint* addresses, int number_of_datatypes, const MPI_Datatype* datatypes)
    : combiner_(combiner)
    , integers_(integers, integers + number_of_integers)
    , addresses_(addresses, addresses + number_of_addresses)
    , datatypes_(datatypes, datatypes + number_of_datatypes)
{
  for (auto& datatype : datatypes_)
    datatype->ref();
}

Datatype_contents::~Datatype_contents()
{
  for (auto& datatype : datatypes_)
    Datatype::unref(datatype);
}

Type_Contiguous::Type_Contiguous(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, MPI_Datatype old_type)
    : Datatype(size, lb, ub, flags), block_count_(block_count), old_type_(old_type)
{
  set_contents(MPI_COMBINER_CONTIGUOUS, 1, &block_count, 0, nullptr, 1, &old_type);
  old_type_->ref();
}

Type_Contiguous::~Type_Contiguous()
{
  Datatype::unref(old_type_);
}

int Type_Contiguous::clone(MPI_Datatype* type)
{
  *type = new Type_Contiguous(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->old_type_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

void Type_Contiguous::serialize(const void* noncontiguous_buf, void* contiguous_buf, int count)
{
  auto* contiguous_buf_char          = static_cast<char*>(contiguous_buf);
  const auto* noncontiguous_buf_char = static_cast<const char*>(noncontiguous_buf) + lb();
  memcpy(contiguous_buf_char, noncontiguous_buf_char, old_type_->size() * count * block_count_);
}

void Type_Contiguous::unserialize(const void* contiguous_buf, void* noncontiguous_buf, int count, MPI_Op op)
{
  const auto* contiguous_buf_char = static_cast<const char*>(contiguous_buf);
  auto* noncontiguous_buf_char    = static_cast<char*>(noncontiguous_buf) + lb();
  int n= count*block_count_;
  if(op!=MPI_OP_NULL)
    op->apply( contiguous_buf_char, noncontiguous_buf_char, &n, old_type_);
}

Type_Hvector::Type_Hvector(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int block_length, MPI_Aint stride, MPI_Datatype old_type): Datatype(size, lb, ub, flags), block_count_(count), block_length_(block_length), block_stride_(stride), old_type_(old_type){
  const std::array<int, 2> ints = {{count, block_length}};
  set_contents(MPI_COMBINER_HVECTOR, 2, ints.data(), 1, &stride, 1, &old_type);
  old_type->ref();
}
Type_Hvector::~Type_Hvector(){
  Datatype::unref(old_type_);
}

int Type_Hvector::clone(MPI_Datatype* type)
{
  *type = new Type_Hvector(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->block_length_, this->block_stride_, this->old_type_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

void Type_Hvector::serialize(const void* noncontiguous_buf, void *contiguous_buf,
                    int count){
  auto* contiguous_buf_char          = static_cast<char*>(contiguous_buf);
  const auto* noncontiguous_buf_char = static_cast<const char*>(noncontiguous_buf);

  for (int i = 0; i < block_count_ * count; i++) {
    if (not(old_type_->flags() & DT_FLAG_DERIVED))
      memcpy(contiguous_buf_char, noncontiguous_buf_char, block_length_ * old_type_->size());
    else
      old_type_->serialize( noncontiguous_buf_char, contiguous_buf_char, block_length_);

    contiguous_buf_char += block_length_*old_type_->size();
    if((i+1)%block_count_ ==0)
      noncontiguous_buf_char += block_length_*old_type_->size();
    else
      noncontiguous_buf_char += block_stride_;
  }
}

void Type_Hvector::unserialize(const void* contiguous_buf, void *noncontiguous_buf,
                              int count, MPI_Op op){
  const auto* contiguous_buf_char = static_cast<const char*>(contiguous_buf);
  auto* noncontiguous_buf_char    = static_cast<char*>(noncontiguous_buf);

  for (int i = 0; i < block_count_ * count; i++) {
    if (not(old_type_->flags() & DT_FLAG_DERIVED)) {
      if(op!=MPI_OP_NULL)
        op->apply( contiguous_buf_char, noncontiguous_buf_char, &block_length_, old_type_);
    }else
      old_type_->unserialize( contiguous_buf_char, noncontiguous_buf_char, block_length_, op);
    contiguous_buf_char += block_length_*old_type_->size();
    if((i+1)%block_count_ ==0)
      noncontiguous_buf_char += block_length_*old_type_->size();
    else
      noncontiguous_buf_char += block_stride_;
  }
}

Type_Vector::Type_Vector(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, int block_length, int stride,
                         MPI_Datatype old_type)
    : Type_Hvector(size, lb, ub, flags, count, block_length, stride * old_type->get_extent(), old_type)
{
  const std::array<int, 3> ints = {{count, block_length, stride}};
  set_contents(MPI_COMBINER_VECTOR, 3, ints.data(), 0, nullptr, 1, &old_type);
}

int Type_Vector::clone(MPI_Datatype* type)
{
  *type = new Type_Vector(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->block_length_, this->block_stride_, this->old_type_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

Type_Hindexed::Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, const int* block_lengths,
                             const MPI_Aint* block_indices, MPI_Datatype old_type)
    : Datatype(size, lb, ub, flags)
    , block_count_(count)
    , block_lengths_(new int[count])
    , block_indices_(new MPI_Aint[count])
    , old_type_(old_type)
{
  std::vector<int> ints(count + 1);
  ints[0]=count;
  for(int i=1;i<=count;i++)
    ints[i]=block_lengths[i-1];
  set_contents(MPI_COMBINER_HINDEXED, count + 1, ints.data(), count, block_indices, 1, &old_type);
  old_type_->ref();
  for (int i = 0; i < count; i++) {
    block_lengths_[i] = block_lengths[i];
    block_indices_[i] = block_indices[i];
  }
}

Type_Hindexed::Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, const int* block_lengths,
                             const int* block_indices, MPI_Datatype old_type, MPI_Aint factor)
    : Datatype(size, lb, ub, flags)
    , block_count_(count)
    , block_lengths_(new int[count])
    , block_indices_(new MPI_Aint[count])
    , old_type_(old_type)
{
  old_type_->ref();
  for (int i = 0; i < count; i++) {
    block_lengths_[i] = block_lengths[i];
    block_indices_[i] = block_indices[i] * factor;
  }
}

int Type_Hindexed::clone(MPI_Datatype* type)
{
  *type = new Type_Hindexed(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->block_lengths_, this->block_indices_, this->old_type_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

Type_Hindexed::~Type_Hindexed()
{
  Datatype::unref(old_type_);
  if(refcount()==0){
    delete[] block_lengths_;
    delete[] block_indices_;
  }
}

void Type_Hindexed::serialize(const void* noncontiguous_buf, void *contiguous_buf,
                int count){
  auto* contiguous_buf_char          = static_cast<char*>(contiguous_buf);
  const auto* noncontiguous_buf_iter = static_cast<const char*>(noncontiguous_buf);
  const auto* noncontiguous_buf_char = noncontiguous_buf_iter + block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (not(old_type_->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_lengths_[i] * old_type_->size());
      else
        old_type_->serialize(noncontiguous_buf_char, contiguous_buf_char,block_lengths_[i]);

      contiguous_buf_char += block_lengths_[i]*old_type_->size();
      if (i<block_count_-1)
        noncontiguous_buf_char = noncontiguous_buf_iter + block_indices_[i+1];
      else
        noncontiguous_buf_char += block_lengths_[i]*old_type_->get_extent();
    }
    noncontiguous_buf_iter=noncontiguous_buf_char;
  }
}

void Type_Hindexed::unserialize(const void* contiguous_buf, void *noncontiguous_buf,
                          int count, MPI_Op op){
  const auto* contiguous_buf_char = static_cast<const char*>(contiguous_buf);
  auto* noncontiguous_buf_char    = static_cast<char*>(noncontiguous_buf) + block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (not(old_type_->flags() & DT_FLAG_DERIVED)) {
        if(op!=MPI_OP_NULL)
          op->apply( contiguous_buf_char, noncontiguous_buf_char, &block_lengths_[i],
                            old_type_);
      }else
        old_type_->unserialize( contiguous_buf_char,noncontiguous_buf_char,block_lengths_[i], op);

      contiguous_buf_char += block_lengths_[i]*old_type_->size();
      if (i<block_count_-1)
        noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf) + block_indices_[i+1];
      else
        noncontiguous_buf_char += block_lengths_[i]*old_type_->get_extent();
    }
    noncontiguous_buf=static_cast<void*>(noncontiguous_buf_char);
  }
}

Type_Indexed::Type_Indexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, const int* block_lengths,
                           const int* block_indices, MPI_Datatype old_type)
    : Type_Hindexed(size, lb, ub, flags, count, block_lengths, block_indices, old_type, old_type->get_extent())
{
  std::vector<int> ints(2 * count + 1);
  ints[0]=count;
  for(int i=1;i<=count;i++)
    ints[i]=block_lengths[i-1];
  for(int i=count+1;i<=2*count;i++)
    ints[i]=block_indices[i-count-1];
  set_contents(MPI_COMBINER_INDEXED, 2 * count + 1, ints.data(), 0, nullptr, 1, &old_type);
}

int Type_Indexed::clone(MPI_Datatype* type)
{
  *type = new Type_Indexed(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->block_lengths_, (int*)(this->block_indices_), this->old_type_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

Type_Struct::Type_Struct(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, const int* block_lengths,
                         const MPI_Aint* block_indices, const MPI_Datatype* old_types)
    : Datatype(size, lb, ub, flags)
    , block_count_(count)
    , block_lengths_(new int[count])
    , block_indices_(new MPI_Aint[count])
    , old_types_(new MPI_Datatype[count])
{
  std::vector<int> ints(count + 1);
  ints[0]=count;
  for(int i=1;i<=count;i++)
    ints[i]=block_lengths[i-1];
  set_contents(MPI_COMBINER_INDEXED, count + 1, ints.data(), count, block_indices, count, old_types);
  for (int i = 0; i < count; i++) {
    block_lengths_[i]=block_lengths[i];
    block_indices_[i]=block_indices[i];
    old_types_[i]=old_types[i];
    old_types_[i]->ref();
  }
}

Type_Struct::~Type_Struct(){
  for (int i = 0; i < block_count_; i++) {
    Datatype::unref(old_types_[i]);
  }
  if(refcount()==0){
    delete[] block_lengths_;
    delete[] block_indices_;
    delete[] old_types_;
  }
}

int Type_Struct::clone(MPI_Datatype* type)
{
  *type = new Type_Struct(this->size(), this->lb(), this->ub(), this->flags(), this->block_count_, this->block_lengths_, this->block_indices_, this->old_types_);
  (*type)->copy_attrs(this);
  return MPI_SUCCESS;
}

void Type_Struct::serialize(const void* noncontiguous_buf, void *contiguous_buf,
                        int count){
  auto* contiguous_buf_char          = static_cast<char*>(contiguous_buf);
  const auto* noncontiguous_buf_iter = static_cast<const char*>(noncontiguous_buf);
  const auto* noncontiguous_buf_char = noncontiguous_buf_iter + block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (not(old_types_[i]->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_lengths_[i] * old_types_[i]->size());
      else
        old_types_[i]->serialize( noncontiguous_buf_char,contiguous_buf_char,block_lengths_[i]);


      contiguous_buf_char += block_lengths_[i]*old_types_[i]->size();
      if (i<block_count_-1)
        noncontiguous_buf_char = noncontiguous_buf_iter + block_indices_[i+1];
      else //let's hope this is MPI_UB ?
        noncontiguous_buf_char += block_lengths_[i]*old_types_[i]->get_extent();
    }
    noncontiguous_buf_iter=noncontiguous_buf_char;
  }
}

void Type_Struct::unserialize(const void* contiguous_buf, void *noncontiguous_buf,
                              int count, MPI_Op op){
  const auto* contiguous_buf_char = static_cast<const char*>(contiguous_buf);
  auto* noncontiguous_buf_char    = static_cast<char*>(noncontiguous_buf) + block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (not(old_types_[i]->flags() & DT_FLAG_DERIVED)) {
        if(op!=MPI_OP_NULL)
          op->apply( contiguous_buf_char, noncontiguous_buf_char, &block_lengths_[i], old_types_[i]);
      }else
        old_types_[i]->unserialize( contiguous_buf_char, noncontiguous_buf_char,block_lengths_[i], op);

      contiguous_buf_char += block_lengths_[i]*old_types_[i]->size();
      if (i<block_count_-1)
        noncontiguous_buf_char =  static_cast<char*>(noncontiguous_buf) + block_indices_[i+1];
      else
        noncontiguous_buf_char += block_lengths_[i]*old_types_[i]->get_extent();
    }
    noncontiguous_buf=static_cast<void*>(noncontiguous_buf_char);
  }
}

}
}
