/* smpi_datatype.cpp -- MPI primitives to handle datatypes                      */
/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "private.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

XBT_LOG_EXTERNAL_CATEGORY(smpi_datatype);

namespace simgrid{
namespace smpi{


Type_Contiguous::Type_Contiguous(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, MPI_Datatype old_type): Datatype(size, lb, ub, flags), block_count_(block_count), old_type_(old_type){
  old_type_->use();
}

Type_Contiguous::~Type_Contiguous(){
  old_type_->unuse();
}

void Type_Contiguous::use(){
  old_type_->use();
};

void Type_Contiguous::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                            int count){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+lb_;
  memcpy(contiguous_buf_char, noncontiguous_buf_char, count * block_count_ * old_type_->size());
}
void Type_Contiguous::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                              int count, MPI_Op op){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+lb_;
  int n= count*block_count_;
  if(op!=MPI_OP_NULL)
    op->apply( contiguous_buf_char, noncontiguous_buf_char, &n, old_type_);
}


Type_Vector::Type_Vector(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int block_length, int stride, MPI_Datatype old_type): Datatype(size, lb, ub, flags), block_count_(count), block_length_(block_length),block_stride_(stride),  old_type_(old_type){
old_type_->use();
}

Type_Vector::~Type_Vector(){
  old_type_->unuse();
}

void Type_Vector::use(){
  old_type_->use();
}


void Type_Vector::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                            int count){
  int i;
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf);

  for (i = 0; i < block_count_ * count; i++) {
      if (!(old_type_->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_length_ * old_type_->size());
      else        
        old_type_->serialize(noncontiguous_buf_char, contiguous_buf_char, block_length_);

    contiguous_buf_char += block_length_*old_type_->size();
    if((i+1)%block_count_ ==0)
      noncontiguous_buf_char += block_length_*old_type_->get_extent();
    else
      noncontiguous_buf_char += block_stride_*old_type_->get_extent();
  }
}

void Type_Vector::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                              int count, MPI_Op op){
  int i;
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf);

  for (i = 0; i < block_count_ * count; i++) {
    if (!(old_type_->flags() & DT_FLAG_DERIVED)){
      if(op != MPI_OP_NULL)
        op->apply(contiguous_buf_char, noncontiguous_buf_char, &block_length_,
          old_type_);
    }else
      old_type_->unserialize(contiguous_buf_char, noncontiguous_buf_char, block_length_, op);

    contiguous_buf_char += block_length_*old_type_->size();
    if((i+1)%block_count_ ==0)
      noncontiguous_buf_char += block_length_*old_type_->get_extent();
    else
      noncontiguous_buf_char += block_stride_*old_type_->get_extent();
  }
}

Type_Hvector::Type_Hvector(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int block_length, MPI_Aint stride, MPI_Datatype old_type): Datatype(size, lb, ub, flags), block_count_(count), block_length_(block_length), block_stride_(stride), old_type_(old_type){
  old_type->use();
}
Type_Hvector::~Type_Hvector(){
  old_type_->unuse();
}
void Type_Hvector::use(){
  old_type_->use();
}

void Type_Hvector::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                    int count){
  int i;
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf);

  for (i = 0; i < block_count_ * count; i++) {
    if (!(old_type_->flags() & DT_FLAG_DERIVED))
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


void Type_Hvector::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                              int count, MPI_Op op){
  int i;
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf);

  for (i = 0; i < block_count_ * count; i++) {
    if (!(old_type_->flags() & DT_FLAG_DERIVED)){
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

Type_Indexed::Type_Indexed(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int* block_lengths, int* block_indices, MPI_Datatype old_type): Datatype(size, lb, ub, flags), block_count_(count), old_type_(old_type){
  old_type->use();
  block_lengths_ = new int[count];
  block_indices_ = new int[count];
  for (int i = 0; i < count; i++) {
    block_lengths_[i]=block_lengths[i];
    block_indices_[i]=block_indices[i];
  }
}

Type_Indexed::~Type_Indexed(){
  old_type_->unuse();
  if(in_use_==0){
    delete[] block_lengths_;
    delete[] block_indices_;
  }
}

void Type_Indexed::use(){
  old_type_->use();
}

void Type_Indexed::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                        int count){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+block_indices_[0] * old_type_->size();
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_type_->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_lengths_[i] * old_type_->size());
      else
        old_type_->serialize( noncontiguous_buf_char, contiguous_buf_char, block_lengths_[i]);

      contiguous_buf_char += block_lengths_[i]*old_type_->size();
      if (i<block_count_-1)
        noncontiguous_buf_char =
          static_cast<char*>(noncontiguous_buf) + block_indices_[i+1]*old_type_->get_extent();
      else
        noncontiguous_buf_char += block_lengths_[i]*old_type_->get_extent();
    }
    noncontiguous_buf=static_cast< void*>(noncontiguous_buf_char);
  }
}


void Type_Indexed::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                      int count, MPI_Op op){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char =
    static_cast<char*>(noncontiguous_buf)+block_indices_[0]*old_type_->get_extent();
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_type_->flags() & DT_FLAG_DERIVED)){
        if(op!=MPI_OP_NULL) 
          op->apply( contiguous_buf_char, noncontiguous_buf_char, &block_lengths_[i],
                    old_type_);
      }else
        old_type_->unserialize( contiguous_buf_char,noncontiguous_buf_char,block_lengths_[i], op);

      contiguous_buf_char += block_lengths_[i]*old_type_->size();
      if (i<block_count_-1)
        noncontiguous_buf_char =
          static_cast<char*>(noncontiguous_buf) + block_indices_[i+1]*old_type_->get_extent();
      else
        noncontiguous_buf_char += block_lengths_[i]*old_type_->get_extent();
    }
    noncontiguous_buf=static_cast<void*>(noncontiguous_buf_char);
  }
}

Type_Hindexed::Type_Hindexed(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int* block_lengths, MPI_Aint* block_indices, MPI_Datatype old_type)
: Datatype(size, lb, ub, flags), block_count_(count), old_type_(old_type)
{
  old_type_->use();
  block_lengths_ = new int[count];
  block_indices_ = new MPI_Aint[count];
  for (int i = 0; i < count; i++) {
    block_lengths_[i]=block_lengths[i];
    block_indices_[i]=block_indices[i];
  }
}

    Type_Hindexed::~Type_Hindexed(){
  old_type_->unuse();
  if(in_use_==0){
    delete[] block_lengths_;
    delete[] block_indices_;
  }
}

void Type_Hindexed::use(){
  old_type_->use();
}
void Type_Hindexed::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                int count){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+ block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_type_->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_lengths_[i] * old_type_->size());
      else
        old_type_->serialize(noncontiguous_buf_char, contiguous_buf_char,block_lengths_[i]);

      contiguous_buf_char += block_lengths_[i]*old_type_->size();
      if (i<block_count_-1)
        noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf) + block_indices_[i+1];
      else
        noncontiguous_buf_char += block_lengths_[i]*old_type_->get_extent();
    }
    noncontiguous_buf=static_cast<void*>(noncontiguous_buf_char);
  }
}

void Type_Hindexed::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                          int count, MPI_Op op){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+ block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_type_->flags() & DT_FLAG_DERIVED)){
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

Type_Struct::Type_Struct(int size,MPI_Aint lb, MPI_Aint ub, int flags, int count, int* block_lengths, MPI_Aint* block_indices, MPI_Datatype* old_types): Datatype(size, lb, ub, flags), block_count_(count), block_lengths_(block_lengths), block_indices_(block_indices), old_types_(old_types){
  block_lengths_= new int[count];
  block_indices_= new MPI_Aint[count];
  old_types_=  new MPI_Datatype[count];
  for (int i = 0; i < count; i++) {
    block_lengths_[i]=block_lengths[i];
    block_indices_[i]=block_indices[i];
    old_types_[i]=old_types[i];
    old_types_[i]->use();
  }
}

Type_Struct::~Type_Struct(){
  for (int i = 0; i < block_count_; i++) {
    old_types_[i]->unuse();
  }
  if(in_use_==0){
    delete[] block_lengths_;
    delete[] block_indices_;
    delete[] old_types_;
  }
}

void Type_Struct::use(){
  for (int i = 0; i < block_count_; i++) {
    old_types_[i]->use();
  }
}

void Type_Struct::serialize( void* noncontiguous_buf, void *contiguous_buf, 
                        int count){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+ block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_types_[i]->flags() & DT_FLAG_DERIVED))
        memcpy(contiguous_buf_char, noncontiguous_buf_char, block_lengths_[i] * old_types_[i]->size());
      else
        old_types_[i]->serialize( noncontiguous_buf_char,contiguous_buf_char,block_lengths_[i]);


      contiguous_buf_char += block_lengths_[i]*old_types_[i]->size();
      if (i<block_count_-1)
        noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf) + block_indices_[i+1];
      else //let's hope this is MPI_UB ?
        noncontiguous_buf_char += block_lengths_[i]*old_types_[i]->get_extent();
    }
    noncontiguous_buf=static_cast<void*>(noncontiguous_buf_char);
  }
}

void Type_Struct::unserialize( void* contiguous_buf, void *noncontiguous_buf, 
                              int count, MPI_Op op){
  char* contiguous_buf_char = static_cast<char*>(contiguous_buf);
  char* noncontiguous_buf_char = static_cast<char*>(noncontiguous_buf)+ block_indices_[0];
  for (int j = 0; j < count; j++) {
    for (int i = 0; i < block_count_; i++) {
      if (!(old_types_[i]->flags() & DT_FLAG_DERIVED)){
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
    noncontiguous_buf=reinterpret_cast<void*>(noncontiguous_buf_char);
  }
}

}
}
