/* smpi_mpi_dt.c -- MPI primitives to handle datatypes                      */
/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

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

//XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi_dt, smpi, "Logging specific to SMPI (datatype)");

//xbt_dict_t smpi_type_keyvals = nullptr;
//int type_keyval_id=0;//avoid collisions


/** Check if the datatype is usable for communications */
bool is_datatype_valid(MPI_Datatype datatype) {
    return datatype != MPI_DATATYPE_NULL && ((datatype->flags_ & DT_FLAG_COMMITED) != 0);
}

size_t smpi_datatype_size(MPI_Datatype datatype)
{
  return datatype->size_;
}

MPI_Aint smpi_datatype_lb(MPI_Datatype datatype)
{
  return datatype->lb_;
}

MPI_Aint smpi_datatype_ub(MPI_Datatype datatype)
{
  return datatype->ub_;
}

int smpi_datatype_dup(MPI_Datatype datatype, MPI_Datatype* new_t)
{
  int ret=MPI_SUCCESS;
  return ret;
}

int smpi_datatype_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return MPI_SUCCESS;
}

MPI_Aint smpi_datatype_get_extent(MPI_Datatype datatype){
  if(datatype == MPI_DATATYPE_NULL){
    return 0;
  }
  return datatype->ub_ - datatype->lb_;
}

void smpi_datatype_get_name(MPI_Datatype datatype, char* name, int* length){
  *length = strlen(datatype->name_);
  strncpy(name, datatype->name_, *length+1);
}

void smpi_datatype_set_name(MPI_Datatype datatype, char* name){
  if(datatype->name_!=nullptr &&  (datatype->flags_ & DT_FLAG_PREDEFINED) == 0)
    xbt_free(datatype->name_);
  datatype->name_ = xbt_strdup(name);
}

int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
 
  return sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
}

/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_vector - output vector
 *  @param noncontiguous_vector - input vector
 *  @param type - pointer containing :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_vector( void* noncontiguous_vector, void *contiguous_vector, int count, void *type)
{
//  s_smpi_mpi_vector_t* type_c = reinterpret_cast<s_smpi_mpi_vector_t*>(type);
//  int i;
//  char* contiguous_vector_char = static_cast<char*>(contiguous_vector);
//  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_vector);

//  for (i = 0; i < type_c->block_count * count; i++) {
//      if (type_c->old_type->sizeof_substruct == 0)
//        memcpy(contiguous_vector_char, noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);
//      else
//        static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->serialize( noncontiguous_vector_char,
//                                                                     contiguous_vector_char,
//                                                                     type_c->block_length, type_c->old_type->substruct);

//    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    if((i+1)%type_c->block_count ==0)
//      noncontiguous_vector_char += type_c->block_length*smpi_datatype_get_extent(type_c->old_type);
//    else
//      noncontiguous_vector_char += type_c->block_stride*smpi_datatype_get_extent(type_c->old_type);
//  }
}

/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_vector - output vector
 *  @param contiguous_vector - input vector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_vector( void* contiguous_vector, void *noncontiguous_vector, int count, void *type, MPI_Op op)
{
//  s_smpi_mpi_vector_t* type_c = reinterpret_cast<s_smpi_mpi_vector_t*>(type);
//  int i;

//  char* contiguous_vector_char = static_cast<char*>(contiguous_vector);
//  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_vector);

//  for (i = 0; i < type_c->block_count * count; i++) {
//    if (type_c->old_type->sizeof_substruct == 0){
//      if(op!=MPI_OP_NULL)
//        op->apply( contiguous_vector_char, noncontiguous_vector_char, &type_c->block_length,
//          &type_c->old_type);
//    }else
//      static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->unserialize(contiguous_vector_char, noncontiguous_vector_char,
//                                                                    type_c->block_length,type_c->old_type->substruct,
//                                                                    op);
//    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    if((i+1)%type_c->block_count ==0)
//      noncontiguous_vector_char += type_c->block_length*smpi_datatype_get_extent(type_c->old_type);
//    else
//      noncontiguous_vector_char += type_c->block_stride*smpi_datatype_get_extent(type_c->old_type);
//  }
}

/* Create a Sub type vector to be able to serialize and unserialize it the structure s_smpi_mpi_vector_t is derived
 * from s_smpi_subtype which required the functions unserialize and serialize */
s_smpi_mpi_vector_t* smpi_datatype_vector_create( int block_stride, int block_length, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype){
  s_smpi_mpi_vector_t *new_t= xbt_new(s_smpi_mpi_vector_t,1);
  new_t->base.serialize = &serialize_vector;
  new_t->base.unserialize = &unserialize_vector;
  new_t->base.subtype_free = &free_vector;
  new_t->base.subtype_use = &use_vector;
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  smpi_datatype_use(old_type);
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

void smpi_datatype_create(MPI_Datatype* new_type, int size,int lb, int ub, int sizeof_substruct, void *struct_type,
                          int flags){
//  MPI_Datatype new_t= xbt_new(s_smpi_mpi_datatype_t,1);
//  new_t->name = nullptr;
//  new_t->size = size;
//  new_t->sizeof_substruct = size>0? sizeof_substruct:0;
//  new_t->lb = lb;
//  new_t->ub = ub;
//  new_t->flags = flags;
//  new_t->substruct = struct_type;
//  new_t->in_use=1;
//  new_t->attributes=nullptr;
//  *new_type = new_t;

//#if HAVE_MC
//  if(MC_is_active())
//    MC_ignore(&(new_t->in_use), sizeof(new_t->in_use));
//#endif
}

void smpi_datatype_free(MPI_Datatype* type){
//  xbt_assert((*type)->in_use_ >= 0);

//  if((*type)->flags & DT_FLAG_PREDEFINED)
//    return;

//  //if still used, mark for deletion
//  if((*type)->in_use_!=0){
//      (*type)->flags_ |=DT_FLAG_DESTROYED;
//      return;
//  }

//  if((*type)->attributes_ !=nullptr){
//    xbt_dict_cursor_t cursor = nullptr;
//    char* key;
//    void * value;
//    int flag;
//    xbt_dict_foreach((*type)->attributes, cursor, key, value){
//      smpi_type_key_elem elem =
//          static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, key, sizeof(int)));
//      if(elem!=nullptr && elem->delete_fn!=nullptr)
//        elem->delete_fn(*type,*key, value, &flag);
//    }
//    xbt_dict_free(&(*type)->attributes);
//  }

//  if ((*type)->sizeof_substruct != 0){
//    //((s_smpi_subtype_t *)(*type)->substruct)->subtype_free(type);  
//    xbt_free((*type)->substruct);
//  }
//  xbt_free((*type)->name);
//  xbt_free(*type);
//  *type = MPI_DATATYPE_NULL;
}

void smpi_datatype_use(MPI_Datatype type){

//  if(type == MPI_DATATYPE_NULL)
//    return;
//  type->in_use++;

//  if(type->sizeof_substruct!=0){
//    static_cast<s_smpi_subtype_t *>((type)->substruct)->subtype_use(&type);  
//  }
//#if HAVE_MC
//  if(MC_is_active())
//    MC_ignore(&(type->in_use), sizeof(type->in_use));
//#endif
}

void smpi_datatype_unuse(MPI_Datatype type)
{
//  if (type == MPI_DATATYPE_NULL)
//    return;

//  if (type->in_use > 0)
//    type->in_use--;

//  if(type->sizeof_substruct!=0){
//    static_cast<s_smpi_subtype_t *>((type)->substruct)->subtype_free(&type);  
//  }

//  if (type->in_use == 0)
//    smpi_datatype_free(&type);

//#if HAVE_MC
//  if(MC_is_active())
//    MC_ignore(&(type->in_use), sizeof(type->in_use));
//#endif
}

/*Contiguous Implementation*/

/* Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hvector - output hvector
 *  @param noncontiguous_hvector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_contiguous( void* noncontiguous_hvector, void *contiguous_hvector, int count, void *type)
{
  s_smpi_mpi_contiguous_t* type_c = reinterpret_cast<s_smpi_mpi_contiguous_t*>(type);
  char* contiguous_vector_char = static_cast<char*>(contiguous_hvector);
  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_hvector)+type_c->lb;
  memcpy(contiguous_vector_char, noncontiguous_vector_char, count* type_c->block_count * type_c->size_oldtype);
}
/* Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_vector - output hvector
 *  @param contiguous_vector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_contiguous(void* contiguous_vector, void *noncontiguous_vector, int count, void *type, MPI_Op op)
{
//  s_smpi_mpi_contiguous_t* type_c = reinterpret_cast<s_smpi_mpi_contiguous_t*>(type);
//  char* contiguous_vector_char = static_cast<char*>(contiguous_vector);
//  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_vector)+type_c->lb;
//  int n= count* type_c->block_count;
//  if(op!=MPI_OP_NULL)
//    op->apply( contiguous_vector_char, noncontiguous_vector_char, &n, &type_c->old_type);
}

void free_contiguous(MPI_Datatype* d){
//  smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_contiguous_t*>((*d)->substruct)->old_type);
}

void use_contiguous(MPI_Datatype* d){
//  smpi_datatype_use(reinterpret_cast<s_smpi_mpi_contiguous_t*>((*d)->substruct)->old_type);
}

/* Create a Sub type contiguous to be able to serialize and unserialize it the structure s_smpi_mpi_contiguous_t is
 * derived from s_smpi_subtype which required the functions unserialize and serialize */
s_smpi_mpi_contiguous_t* smpi_datatype_contiguous_create( MPI_Aint lb, int block_count, MPI_Datatype old_type,
                                                  int size_oldtype){
  if(block_count==0)
    return nullptr;
  s_smpi_mpi_contiguous_t *new_t= xbt_new(s_smpi_mpi_contiguous_t,1);
  new_t->base.serialize = &serialize_contiguous;
  new_t->base.unserialize = &unserialize_contiguous;
  new_t->base.subtype_free = &free_contiguous;
  new_t->base.subtype_use = &use_contiguous;
  new_t->lb = lb;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  smpi_datatype_use(old_type);
  new_t->size_oldtype = size_oldtype;
  smpi_datatype_use(old_type);
  return new_t;
}

int smpi_datatype_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type, MPI_Aint lb)
{
  int retval;
//  if(old_type->sizeof_substruct){
//    //handle this case as a hvector with stride equals to the extent of the datatype
//    return smpi_datatype_hvector(count, 1, smpi_datatype_get_extent(old_type), old_type, new_type);
//  }
//  
//  s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb, count, old_type,smpi_datatype_size(old_type));

//  smpi_datatype_create(new_type, count * smpi_datatype_size(old_type),lb,lb + count * smpi_datatype_size(old_type),
//            sizeof(s_smpi_mpi_contiguous_t),subtype, DT_FLAG_CONTIGUOUS);
  retval=MPI_SUCCESS;
  return retval;
}

int smpi_datatype_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
//  if (blocklen<0) 
//    return MPI_ERR_ARG;
//  MPI_Aint lb = 0;
//  MPI_Aint ub = 0;
//  if(count>0){
//    lb=smpi_datatype_lb(old_type);
//    ub=((count-1)*stride+blocklen-1)*smpi_datatype_get_extent(old_type)+smpi_datatype_ub(old_type);
//  }
//  if(old_type->sizeof_substruct != 0 || stride != blocklen){

//    s_smpi_mpi_vector_t* subtype = smpi_datatype_vector_create(stride, blocklen, count, old_type,
//                                                                smpi_datatype_size(old_type));
//    smpi_datatype_create(new_type, count * (blocklen) * smpi_datatype_size(old_type), lb, ub, sizeof(s_smpi_mpi_vector_t), subtype,
//                         DT_FLAG_VECTOR);
//    retval=MPI_SUCCESS;
//  }else{
//    /* in this situation the data are contiguous thus it's not required to serialize and unserialize it*/
//    smpi_datatype_create(new_type, count * blocklen * smpi_datatype_size(old_type), 0, ((count -1) * stride + blocklen)*
//                         smpi_datatype_size(old_type), 0, nullptr, DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
//  }
  return retval;
}

void free_vector(MPI_Datatype* d){
//  smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_indexed_t*>((*d)->substruct)->old_type);
}

void use_vector(MPI_Datatype* d){
//  smpi_datatype_use(reinterpret_cast<s_smpi_mpi_indexed_t*>((*d)->substruct)->old_type);
}

/* Hvector Implementation - Vector with stride in bytes */

/* Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hvector - output hvector
 *  @param noncontiguous_hvector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_hvector( void* noncontiguous_hvector, void *contiguous_hvector, int count, void *type)
{
//  s_smpi_mpi_hvector_t* type_c = reinterpret_cast<s_smpi_mpi_hvector_t*>(type);
//  int i;
//  char* contiguous_vector_char = static_cast<char*>(contiguous_hvector);
//  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_hvector);

//  for (i = 0; i < type_c->block_count * count; i++) {
//    if (type_c->old_type->sizeof_substruct == 0)
//      memcpy(contiguous_vector_char, noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);
//    else
//      static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->serialize( noncontiguous_vector_char,
//                                                                   contiguous_vector_char,
//                                                                   type_c->block_length, type_c->old_type->substruct);

//    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    if((i+1)%type_c->block_count ==0)
//      noncontiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    else
//      noncontiguous_vector_char += type_c->block_stride;
//  }
}
/* Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_vector - output hvector
 *  @param contiguous_vector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_hvector( void* contiguous_vector, void *noncontiguous_vector, int count, void *type, MPI_Op op)
{
//  s_smpi_mpi_hvector_t* type_c = reinterpret_cast<s_smpi_mpi_hvector_t*>(type);
//  int i;

//  char* contiguous_vector_char = static_cast<char*>(contiguous_vector);
//  char* noncontiguous_vector_char = static_cast<char*>(noncontiguous_vector);

//  for (i = 0; i < type_c->block_count * count; i++) {
//    if (type_c->old_type->sizeof_substruct == 0){
//      if(op!=MPI_OP_NULL) 
//        op->apply( contiguous_vector_char, noncontiguous_vector_char, &type_c->block_length, &type_c->old_type);
//    }else
//      static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->unserialize( contiguous_vector_char, noncontiguous_vector_char,
//                                                                     type_c->block_length, type_c->old_type->substruct,
//                                                                     op);
//    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    if((i+1)%type_c->block_count ==0)
//      noncontiguous_vector_char += type_c->block_length*type_c->size_oldtype;
//    else
//      noncontiguous_vector_char += type_c->block_stride;
//  }
}

/* Create a Sub type vector to be able to serialize and unserialize it the structure s_smpi_mpi_vector_t is derived
 * from s_smpi_subtype which required the functions unserialize and serialize
 *
 */
s_smpi_mpi_hvector_t* smpi_datatype_hvector_create( MPI_Aint block_stride, int block_length, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype){
  s_smpi_mpi_hvector_t *new_t= xbt_new(s_smpi_mpi_hvector_t,1);
  new_t->base.serialize = &serialize_hvector;
  new_t->base.unserialize = &unserialize_hvector;
  new_t->base.subtype_free = &free_hvector;
  new_t->base.subtype_use = &use_hvector;
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  smpi_datatype_use(old_type);
  return new_t;
}

//do nothing for vector types
void free_hvector(MPI_Datatype* d){
//  smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_hvector_t*>((*d)->substruct)->old_type);
}

void use_hvector(MPI_Datatype* d){
//  smpi_datatype_use(reinterpret_cast<s_smpi_mpi_hvector_t*>((*d)->substruct)->old_type);
}

int smpi_datatype_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
//  if (blocklen<0) 
//    return MPI_ERR_ARG;
//  MPI_Aint lb = 0;
//  MPI_Aint ub = 0;
//  if(count>0){
//    lb=smpi_datatype_lb(old_type);
//    ub=((count-1)*stride)+(blocklen-1)*smpi_datatype_get_extent(old_type)+smpi_datatype_ub(old_type);
//  }
//  if(old_type->sizeof_substruct != 0 || stride != blocklen*smpi_datatype_get_extent(old_type)){
//    s_smpi_mpi_hvector_t* subtype = smpi_datatype_hvector_create( stride, blocklen, count, old_type,
//                                                                  smpi_datatype_size(old_type));

//    smpi_datatype_create(new_type, count * blocklen * smpi_datatype_size(old_type), lb,ub, sizeof(s_smpi_mpi_hvector_t), subtype, DT_FLAG_VECTOR);
//    retval=MPI_SUCCESS;
//  }else{
//    smpi_datatype_create(new_type, count * blocklen * smpi_datatype_size(old_type),0,count * blocklen *
//                                             smpi_datatype_size(old_type), 0, nullptr, DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
  retval=MPI_SUCCESS;
//  }
  return retval;
}

/* Indexed Implementation */

/* Copies noncontiguous data into contiguous memory.
 *  @param contiguous_indexed - output indexed
 *  @param noncontiguous_indexed - input indexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in element
 *      - count - the number of rows of matrix
 */
void serialize_indexed( void* noncontiguous_indexed, void *contiguous_indexed, int count, void *type)
{
//  s_smpi_mpi_indexed_t* type_c = reinterpret_cast<s_smpi_mpi_indexed_t*>(type);
//  char* contiguous_indexed_char = static_cast<char*>(contiguous_indexed);
//  char* noncontiguous_indexed_char = static_cast<char*>(noncontiguous_indexed)+type_c->block_indices[0] * type_c->size_oldtype;
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_type->sizeof_substruct == 0)
//        memcpy(contiguous_indexed_char, noncontiguous_indexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
//      else
//        static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->serialize( noncontiguous_indexed_char,
//                                                                     contiguous_indexed_char,
//                                                                     type_c->block_lengths[i],
//                                                                     type_c->old_type->substruct);

//      contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
//      if (i<type_c->block_count-1)
//        noncontiguous_indexed_char =
//          static_cast<char*>(noncontiguous_indexed) + type_c->block_indices[i+1]*smpi_datatype_get_extent(type_c->old_type);
//      else
//        noncontiguous_indexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
//    }
//    noncontiguous_indexed=static_cast< void*>(noncontiguous_indexed_char);
//  }
}
/* Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_indexed - output indexed
 *  @param contiguous_indexed - input indexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in element
 *      - count - the number of rows of matrix
 */
void unserialize_indexed( void* contiguous_indexed, void *noncontiguous_indexed, int count, void *type, MPI_Op op)
{
//  s_smpi_mpi_indexed_t* type_c = reinterpret_cast<s_smpi_mpi_indexed_t*>(type);
//  char* contiguous_indexed_char = static_cast<char*>(contiguous_indexed);
//  char* noncontiguous_indexed_char =
//    static_cast<char*>(noncontiguous_indexed)+type_c->block_indices[0]*smpi_datatype_get_extent(type_c->old_type);
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_type->sizeof_substruct == 0){
//        if(op!=MPI_OP_NULL) 
//          op->apply( contiguous_indexed_char, noncontiguous_indexed_char, &type_c->block_lengths[i],
//                    &type_c->old_type);
//      }else
//        static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->unserialize( contiguous_indexed_char,
//                                                                       noncontiguous_indexed_char,
//                                                                       type_c->block_lengths[i],
//                                                                       type_c->old_type->substruct, op);

//      contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
//      if (i<type_c->block_count-1)
//        noncontiguous_indexed_char =
//          static_cast<char*>(noncontiguous_indexed) + type_c->block_indices[i+1]*smpi_datatype_get_extent(type_c->old_type);
//      else
//        noncontiguous_indexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
//    }
//    noncontiguous_indexed=static_cast<void*>(noncontiguous_indexed_char);
//  }
}

void free_indexed(MPI_Datatype* type){
//  if((*type)->in_use==0){
//    xbt_free(reinterpret_cast<s_smpi_mpi_indexed_t*>((*type)->substruct)->block_lengths);
//    xbt_free(reinterpret_cast<s_smpi_mpi_indexed_t*>((*type)->substruct)->block_indices);
//  }
//  smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_indexed_t*>((*type)->substruct)->old_type);
}

void use_indexed(MPI_Datatype* type){
//  smpi_datatype_use(reinterpret_cast<s_smpi_mpi_indexed_t*>((*type)->substruct)->old_type);
}


/* Create a Sub type indexed to be able to serialize and unserialize it the structure s_smpi_mpi_indexed_t is derived
 * from s_smpi_subtype which required the functions unserialize and serialize */
s_smpi_mpi_indexed_t* smpi_datatype_indexed_create( int* block_lengths, int* block_indices, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype){
  s_smpi_mpi_indexed_t *new_t= xbt_new(s_smpi_mpi_indexed_t,1);
  new_t->base.serialize = &serialize_indexed;
  new_t->base.unserialize = &unserialize_indexed;
  new_t->base.subtype_free = &free_indexed;
  new_t->base.subtype_use = &use_indexed;
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(int, block_count);
  for (int i = 0; i < block_count; i++) {
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
  }
  new_t->block_count = block_count;
  smpi_datatype_use(old_type);
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

int smpi_datatype_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
//  int size = 0;
//  bool contiguous=true;
//  MPI_Aint lb = 0;
//  MPI_Aint ub = 0;
//  if(count>0){
//    lb=indices[0]*smpi_datatype_get_extent(old_type);
//    ub=indices[0]*smpi_datatype_get_extent(old_type) + blocklens[0]*smpi_datatype_ub(old_type);
//  }

//  for (int i = 0; i < count; i++) {
//    if (blocklens[i] < 0)
//      return MPI_ERR_ARG;
//    size += blocklens[i];

//    if(indices[i]*smpi_datatype_get_extent(old_type)+smpi_datatype_lb(old_type)<lb)
//      lb = indices[i]*smpi_datatype_get_extent(old_type)+smpi_datatype_lb(old_type);
//    if(indices[i]*smpi_datatype_get_extent(old_type)+blocklens[i]*smpi_datatype_ub(old_type)>ub)
//      ub = indices[i]*smpi_datatype_get_extent(old_type)+blocklens[i]*smpi_datatype_ub(old_type);

//    if ( (i< count -1) && (indices[i]+blocklens[i] != indices[i+1]) )
//      contiguous=false;
//  }
//  if (old_type->sizeof_substruct != 0)
//    contiguous=false;

//  if(!contiguous){
//    s_smpi_mpi_indexed_t* subtype = smpi_datatype_indexed_create( blocklens, indices, count, old_type,
//                                                                  smpi_datatype_size(old_type));
//     smpi_datatype_create(new_type,  size * smpi_datatype_size(old_type),lb,ub,sizeof(s_smpi_mpi_indexed_t), subtype, DT_FLAG_DATA);
//  }else{
//    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb, size, old_type,
//                                                                  smpi_datatype_size(old_type));
//    smpi_datatype_create(new_type, size * smpi_datatype_size(old_type), lb, ub, sizeof(s_smpi_mpi_contiguous_t), subtype,
//                         DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
//  }
  return MPI_SUCCESS;
}
/* Hindexed Implementation - Indexed with indices in bytes */

/* Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hindexed - output hindexed
 *  @param noncontiguous_hindexed - input hindexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in bytes
 *      - count - the number of rows of matrix
 */
void serialize_hindexed( void* noncontiguous_hindexed, void *contiguous_hindexed, int count, void *type)
{
//  s_smpi_mpi_hindexed_t* type_c = reinterpret_cast<s_smpi_mpi_hindexed_t*>(type);
//  char* contiguous_hindexed_char = static_cast<char*>(contiguous_hindexed);
//  char* noncontiguous_hindexed_char = static_cast<char*>(noncontiguous_hindexed)+ type_c->block_indices[0];
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_type->sizeof_substruct == 0)
//        memcpy(contiguous_hindexed_char, noncontiguous_hindexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
//      else
//        static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->serialize( noncontiguous_hindexed_char,
//                                                                     contiguous_hindexed_char,
//                                                                     type_c->block_lengths[i],
//                                                                     type_c->old_type->substruct);

//      contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
//      if (i<type_c->block_count-1)
//        noncontiguous_hindexed_char = static_cast<char*>(noncontiguous_hindexed) + type_c->block_indices[i+1];
//      else
//        noncontiguous_hindexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
//    }
//    noncontiguous_hindexed=static_cast<void*>(noncontiguous_hindexed_char);
//  }
}
/* Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_hindexed - output hindexed
 *  @param contiguous_hindexed - input hindexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in bytes
 *      - count - the number of rows of matrix
 */
void unserialize_hindexed( void* contiguous_hindexed, void *noncontiguous_hindexed, int count, void *type,
                         MPI_Op op)
{
//  s_smpi_mpi_hindexed_t* type_c = reinterpret_cast<s_smpi_mpi_hindexed_t*>(type);

//  char* contiguous_hindexed_char = static_cast<char*>(contiguous_hindexed);
//  char* noncontiguous_hindexed_char = static_cast<char*>(noncontiguous_hindexed)+ type_c->block_indices[0];
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_type->sizeof_substruct == 0){
//        if(op!=MPI_OP_NULL) 
//          op->apply( contiguous_hindexed_char, noncontiguous_hindexed_char, &type_c->block_lengths[i],
//                            &type_c->old_type);
//      }else
//        static_cast<s_smpi_subtype_t*>(type_c->old_type->substruct)->unserialize( contiguous_hindexed_char,
//                                                                       noncontiguous_hindexed_char,
//                                                                       type_c->block_lengths[i],
//                                                                       type_c->old_type->substruct, op);

//      contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
//      if (i<type_c->block_count-1)
//        noncontiguous_hindexed_char = static_cast<char*>(noncontiguous_hindexed) + type_c->block_indices[i+1];
//      else
//        noncontiguous_hindexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
//    }
//    noncontiguous_hindexed=static_cast<void*>(noncontiguous_hindexed_char);
//  }
}

void free_hindexed(MPI_Datatype* type){
//  if((*type)->in_use==0){
//    xbt_free(reinterpret_cast<s_smpi_mpi_hindexed_t*>((*type)->substruct)->block_lengths);
//    xbt_free(reinterpret_cast<s_smpi_mpi_hindexed_t*>((*type)->substruct)->block_indices);
//  }
//  smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_hindexed_t*>((*type)->substruct)->old_type);
}

void use_hindexed(MPI_Datatype* type){
//  smpi_datatype_use(reinterpret_cast<s_smpi_mpi_hindexed_t*>((*type)->substruct)->old_type);
}

/* Create a Sub type hindexed to be able to serialize and unserialize it the structure s_smpi_mpi_hindexed_t is derived
 * from s_smpi_subtype which required the functions unserialize and serialize
 */
s_smpi_mpi_hindexed_t* smpi_datatype_hindexed_create( int* block_lengths, MPI_Aint* block_indices, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype){
  s_smpi_mpi_hindexed_t *new_t= xbt_new(s_smpi_mpi_hindexed_t,1);
  new_t->base.serialize = &serialize_hindexed;
  new_t->base.unserialize = &unserialize_hindexed;
  new_t->base.subtype_free = &free_hindexed;
  new_t->base.subtype_use = &use_hindexed;
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(MPI_Aint, block_count);
  for(int i=0;i<block_count;i++){
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
  }
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  smpi_datatype_use(old_type);
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

int smpi_datatype_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
//  int size = 0;
//  bool contiguous=true;
//  MPI_Aint lb = 0;
//  MPI_Aint ub = 0;
//  if(count>0){
//    lb=indices[0] + smpi_datatype_lb(old_type);
//    ub=indices[0] + blocklens[0]*smpi_datatype_ub(old_type);
//  }
//  for (int i = 0; i < count; i++) {
//    if (blocklens[i] < 0)
//      return MPI_ERR_ARG;
//    size += blocklens[i];

//    if(indices[i]+smpi_datatype_lb(old_type)<lb) 
//      lb = indices[i]+smpi_datatype_lb(old_type);
//    if(indices[i]+blocklens[i]*smpi_datatype_ub(old_type)>ub) 
//      ub = indices[i]+blocklens[i]*smpi_datatype_ub(old_type);

//    if ( (i< count -1) && (indices[i]+blocklens[i]*(static_cast<int>(smpi_datatype_size(old_type))) != indices[i+1]) )
//      contiguous=false;
//  }
//  if (old_type->sizeof_substruct != 0 || lb!=0)
//    contiguous=false;

//  if(!contiguous){
//    s_smpi_mpi_hindexed_t* subtype = smpi_datatype_hindexed_create( blocklens, indices, count, old_type,
//                                                                  smpi_datatype_size(old_type));
//    smpi_datatype_create(new_type,  size * smpi_datatype_size(old_type), lb, ub ,sizeof(s_smpi_mpi_hindexed_t), subtype, DT_FLAG_DATA);
//  }else{
//    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create(lb,size, old_type, smpi_datatype_size(old_type));
//    smpi_datatype_create(new_type,  size * smpi_datatype_size(old_type), 0,size * smpi_datatype_size(old_type),
//               1, subtype, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
//  }
  return MPI_SUCCESS;
}

/* struct Implementation - Indexed with indices in bytes */

/* Copies noncontiguous data into contiguous memory.
 *  @param contiguous_struct - output struct
 *  @param noncontiguous_struct - input struct
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_struct( void* noncontiguous_struct, void *contiguous_struct, int count, void *type)
{
//  s_smpi_mpi_struct_t* type_c = reinterpret_cast<s_smpi_mpi_struct_t*>(type);
//  char* contiguous_struct_char = static_cast<char*>(contiguous_struct);
//  char* noncontiguous_struct_char = static_cast<char*>(noncontiguous_struct)+ type_c->block_indices[0];
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_types[i]->sizeof_substruct == 0)
//        memcpy(contiguous_struct_char, noncontiguous_struct_char,
//               type_c->block_lengths[i] * smpi_datatype_size(type_c->old_types[i]));
//      else
//        static_cast<s_smpi_subtype_t*>(type_c->old_types[i]->substruct)->serialize( noncontiguous_struct_char,
//                                                                         contiguous_struct_char,
//                                                                         type_c->block_lengths[i],
//                                                                         type_c->old_types[i]->substruct);


//      contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
//      if (i<type_c->block_count-1)
//        noncontiguous_struct_char = static_cast<char*>(noncontiguous_struct) + type_c->block_indices[i+1];
//      else //let's hope this is MPI_UB ?
//        noncontiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_types[i]);
//    }
//    noncontiguous_struct=static_cast<void*>(noncontiguous_struct_char);
//  }
}

/* Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_struct - output struct
 *  @param contiguous_struct - input struct
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_struct( void* contiguous_struct, void *noncontiguous_struct, int count, void *type, MPI_Op op)
{
//  s_smpi_mpi_struct_t* type_c = reinterpret_cast<s_smpi_mpi_struct_t*>(type);

//  char* contiguous_struct_char = static_cast<char*>(contiguous_struct);
//  char* noncontiguous_struct_char = static_cast<char*>(noncontiguous_struct)+ type_c->block_indices[0];
//  for (int j = 0; j < count; j++) {
//    for (int i = 0; i < type_c->block_count; i++) {
//      if (type_c->old_types[i]->sizeof_substruct == 0){
//        if(op!=MPI_OP_NULL) 
//          op->apply( contiguous_struct_char, noncontiguous_struct_char, &type_c->block_lengths[i],
//           & type_c->old_types[i]);
//      }else
//        static_cast<s_smpi_subtype_t*>(type_c->old_types[i]->substruct)->unserialize( contiguous_struct_char,
//                                                                           noncontiguous_struct_char,
//                                                                           type_c->block_lengths[i],
//                                                                           type_c->old_types[i]->substruct, op);

//      contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
//      if (i<type_c->block_count-1)
//        noncontiguous_struct_char =  static_cast<char*>(noncontiguous_struct) + type_c->block_indices[i+1];
//      else
//        noncontiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_types[i]);
//    }
//    noncontiguous_struct=reinterpret_cast<void*>(noncontiguous_struct_char);
//  }
}

void free_struct(MPI_Datatype* type){
//  for (int i = 0; i < reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->block_count; i++)
//    smpi_datatype_unuse(reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->old_types[i]);
//  if((*type)->in_use==0){
//    xbt_free(reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->block_lengths);
//    xbt_free(reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->block_indices);
//    xbt_free(reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->old_types);
//  }
}

void use_struct(MPI_Datatype* type){
//  for (int i = 0; i < reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->block_count; i++)
//    smpi_datatype_use(reinterpret_cast<s_smpi_mpi_struct_t*>((*type)->substruct)->old_types[i]);
}

/* Create a Sub type struct to be able to serialize and unserialize it the structure s_smpi_mpi_struct_t is derived
 * from s_smpi_subtype which required the functions unserialize and serialize
 */
s_smpi_mpi_struct_t* smpi_datatype_struct_create( int* block_lengths, MPI_Aint* block_indices, int block_count,
                                                  MPI_Datatype* old_types){
  s_smpi_mpi_struct_t *new_t= xbt_new(s_smpi_mpi_struct_t,1);
  new_t->base.serialize = &serialize_struct;
  new_t->base.unserialize = &unserialize_struct;
  new_t->base.subtype_free = &free_struct;
  new_t->base.subtype_use = &use_struct;
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(MPI_Aint, block_count);
  new_t->old_types=  xbt_new(MPI_Datatype, block_count);
  for (int i = 0; i < block_count; i++) {
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
    new_t->old_types[i]=old_types[i];
    smpi_datatype_use(new_t->old_types[i]);
  }
  new_t->block_count = block_count;
  return new_t;
}

int smpi_datatype_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type)
{
//  size_t size = 0;
//  bool contiguous=true;
//  size = 0;
//  MPI_Aint lb = 0;
//  MPI_Aint ub = 0;
//  if(count>0){
//    lb=indices[0] + smpi_datatype_lb(old_types[0]);
//    ub=indices[0] + blocklens[0]*smpi_datatype_ub(old_types[0]);
//  }
//  bool forced_lb=false;
//  bool forced_ub=false;
//  for (int i = 0; i < count; i++) {
//    if (blocklens[i]<0)
//      return MPI_ERR_ARG;
//    if (old_types[i]->sizeof_substruct != 0)
//      contiguous=false;

//    size += blocklens[i]*smpi_datatype_size(old_types[i]);
//    if (old_types[i]==MPI_LB){
//      lb=indices[i];
//      forced_lb=true;
//    }
//    if (old_types[i]==MPI_UB){
//      ub=indices[i];
//      forced_ub=true;
//    }

//    if(!forced_lb && indices[i]+smpi_datatype_lb(old_types[i])<lb) 
//      lb = indices[i];
//    if(!forced_ub &&  indices[i]+blocklens[i]*smpi_datatype_ub(old_types[i])>ub)
//      ub = indices[i]+blocklens[i]*smpi_datatype_ub(old_types[i]);

//    if ( (i< count -1) && (indices[i]+blocklens[i]*static_cast<int>(smpi_datatype_size(old_types[i])) != indices[i+1]) )
//      contiguous=false;
//  }

//  if(!contiguous){
//    s_smpi_mpi_struct_t* subtype = smpi_datatype_struct_create( blocklens, indices, count, old_types);

//    smpi_datatype_create(new_type,  size, lb, ub,sizeof(s_smpi_mpi_struct_t), subtype, DT_FLAG_DATA);
//  }else{
//    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb, size, MPI_CHAR, 1);
//    smpi_datatype_create(new_type,  size, lb, ub,1, subtype, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
//  }
  return MPI_SUCCESS;
}

void smpi_datatype_commit(MPI_Datatype *datatype)
{
//  (*datatype)->flags=  ((*datatype)->flags | DT_FLAG_COMMITED);
}


int smpi_type_attr_delete(MPI_Datatype type, int keyval){
//  smpi_type_key_elem elem =
//    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
//  if(elem==nullptr)
//    return MPI_ERR_ARG;
//  if(elem->delete_fn!=MPI_NULL_DELETE_FN){
//    void * value = nullptr;
//    int flag;
//    if(smpi_type_attr_get(type, keyval, &value, &flag)==MPI_SUCCESS){
//      int ret = elem->delete_fn(type, keyval, value, &flag);
//      if(ret!=MPI_SUCCESS) 
//        return ret;
//    }
//  }  
//  if(type->attributes==nullptr)
//    return MPI_ERR_ARG;

//  xbt_dict_remove_ext(type->attributes, reinterpret_cast<const char*>(&keyval), sizeof(int));
  return MPI_SUCCESS;
}

int smpi_type_attr_get(MPI_Datatype type, int keyval, void* attr_value, int* flag){
//  smpi_type_key_elem elem =
//    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
//  if(elem==nullptr)
//    return MPI_ERR_ARG;
//  if(type->attributes==nullptr){
//    *flag=0;
//    return MPI_SUCCESS;
//  }
//  try {
//    *static_cast<void**>(attr_value) = xbt_dict_get_ext(type->attributes, reinterpret_cast<const char*>(&keyval), sizeof(int));
//    *flag=1;
//  }
//  catch (xbt_ex& ex) {
//    *flag=0;
//  }
  return MPI_SUCCESS;
}

int smpi_type_attr_put(MPI_Datatype type, int keyval, void* attr_value){
//  if(smpi_type_keyvals==nullptr)
//    smpi_type_keyvals = xbt_dict_new_homogeneous(nullptr);
//  smpi_type_key_elem elem =
//     static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
//  if(elem==nullptr)
//    return MPI_ERR_ARG;
//  int flag;
//  void* value = nullptr;
//  smpi_type_attr_get(type, keyval, &value, &flag);
//  if(flag!=0 && elem->delete_fn!=MPI_NULL_DELETE_FN){
//    int ret = elem->delete_fn(type, keyval, value, &flag);
//    if(ret!=MPI_SUCCESS) 
//      return ret;
//  }
//  if(type->attributes==nullptr)
//    type->attributes = xbt_dict_new_homogeneous(nullptr);

//  xbt_dict_set_ext(type->attributes, reinterpret_cast<const char*>(&keyval), sizeof(int), attr_value, nullptr);
  return MPI_SUCCESS;
}

int smpi_type_keyval_create(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state){
//  if(smpi_type_keyvals==nullptr)
//    smpi_type_keyvals = xbt_dict_new_homogeneous(nullptr);

//  smpi_type_key_elem value = (smpi_type_key_elem) xbt_new0(s_smpi_mpi_type_key_elem_t,1);

//  value->copy_fn=copy_fn;
//  value->delete_fn=delete_fn;

//  *keyval = type_keyval_id;
//  xbt_dict_set_ext(smpi_type_keyvals,reinterpret_cast<const char*>(keyval), sizeof(int),reinterpret_cast<void*>(value), nullptr);
//  type_keyval_id++;
  return MPI_SUCCESS;
}

int smpi_type_keyval_free(int* keyval){
//  smpi_type_key_elem elem =
//    static_cast<smpi_type_key_elem>(xbt_dict_get_or_null_ext(smpi_type_keyvals, reinterpret_cast<const char*>(keyval), sizeof(int)));
//  if(elem==0){
//    return MPI_ERR_ARG;
//  }
//  xbt_dict_remove_ext(smpi_type_keyvals, reinterpret_cast<const char*>(keyval), sizeof(int));
//  xbt_free(elem);
  return MPI_SUCCESS;
}

int smpi_mpi_pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position,MPI_Comm comm){
  size_t size = smpi_datatype_size(type);
  if (outcount - *position < incount*static_cast<int>(size))
    return MPI_ERR_BUFFER;
  smpi_datatype_copy(inbuf, incount, type, static_cast<char*>(outbuf) + *position, outcount, MPI_CHAR);
  *position += incount * size;
  return MPI_SUCCESS;
}

int smpi_mpi_unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type,MPI_Comm comm){
  int size = static_cast<int>(smpi_datatype_size(type));
  if (outcount*size> insize)
    return MPI_ERR_BUFFER;
  smpi_datatype_copy(static_cast<char*>(inbuf) + *position, insize, MPI_CHAR, outbuf, outcount, type);
  *position += outcount * size;
  return MPI_SUCCESS;
}
