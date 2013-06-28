/* smpi_mpi_dt.c -- MPI primitives to handle datatypes                        */
/* FIXME: a very incomplete implementation                                    */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi_dt, smpi,
                                "Logging specific to SMPI (datatype)");

#define CREATE_MPI_DATATYPE(name, type)       \
  static s_smpi_mpi_datatype_t mpi_##name = { \
    sizeof(type),  /* size */                 \
    0,             /*was 1 has_subtype*/             \
    0,             /* lb */                   \
    sizeof(type),  /* ub = lb + size */       \
    DT_FLAG_BASIC,  /* flags */              \
    NULL           /* pointer on extended struct*/ \
  };                                          \
MPI_Datatype name = &mpi_##name;

#define CREATE_MPI_DATATYPE_NULL(name)       \
  static s_smpi_mpi_datatype_t mpi_##name = { \
    0,  /* size */                 \
    0,             /*was 1 has_subtype*/             \
    0,             /* lb */                   \
    0,  /* ub = lb + size */       \
    DT_FLAG_BASIC,  /* flags */              \
    NULL           /* pointer on extended struct*/ \
  };                                          \
MPI_Datatype name = &mpi_##name;

//The following are datatypes for the MPI functions MPI_MAXLOC and MPI_MINLOC.
typedef struct {
  float value;
  int index;
} float_int;
typedef struct {
  float value;
  float index;
} float_float;
typedef struct {
  double value;
  double index;
} double_double;
typedef struct {
  long value;
  int index;
} long_int;
typedef struct {
  double value;
  int index;
} double_int;
typedef struct {
  short value;
  int index;
} short_int;
typedef struct {
  int value;
  int index;
} int_int;
typedef struct {
  long double value;
  int index;
} long_double_int;

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
CREATE_MPI_DATATYPE(MPI_C_BOOL, _Bool);
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

CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE_INT, long_double_int);

CREATE_MPI_DATATYPE_NULL(MPI_UB);
CREATE_MPI_DATATYPE_NULL(MPI_LB);
CREATE_MPI_DATATYPE_NULL(MPI_PACKED);
// Internal use only
CREATE_MPI_DATATYPE(MPI_PTR, void*);


size_t smpi_datatype_size(MPI_Datatype datatype)
{
  return datatype->size;
}



MPI_Aint smpi_datatype_lb(MPI_Datatype datatype)
{
  return datatype->lb;
}

MPI_Aint smpi_datatype_ub(MPI_Datatype datatype)
{
  return datatype->ub;
}

int smpi_datatype_extent(MPI_Datatype datatype, MPI_Aint * lb,
                         MPI_Aint * extent)
{
  *lb = datatype->lb;
  *extent = datatype->ub - datatype->lb;
  return MPI_SUCCESS;
}

MPI_Aint smpi_datatype_get_extent(MPI_Datatype datatype){
  return datatype->ub - datatype->lb;
}

int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
  int count;

  /* First check if we really have something to do */
  if (recvcount > 0 && recvbuf != sendbuf) {
    /* FIXME: treat packed cases */
    sendcount *= smpi_datatype_size(sendtype);
    recvcount *= smpi_datatype_size(recvtype);
    count = sendcount < recvcount ? sendcount : recvcount;

    if(sendtype->has_subtype == 0 && recvtype->has_subtype == 0) {
      memcpy(recvbuf, sendbuf, count);
    }
    else if (sendtype->has_subtype == 0)
    {
      s_smpi_subtype_t *subtype =  recvtype->substruct;
      subtype->unserialize( sendbuf, recvbuf,1, subtype);
    }
    else if (recvtype->has_subtype == 0)
    {
      s_smpi_subtype_t *subtype =  sendtype->substruct;
      subtype->serialize(sendbuf, recvbuf,1, subtype);
    }else{
      s_smpi_subtype_t *subtype =  sendtype->substruct;


      void * buf_tmp = xbt_malloc(count);

      subtype->serialize( sendbuf, buf_tmp,1, subtype);
      subtype =  recvtype->substruct;
      subtype->unserialize( buf_tmp, recvbuf,1, subtype);

      free(buf_tmp);
    }
  }

  return sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
}

/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_vector - output vector
 *  @param noncontiguous_vector - input vector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_vector( const void *noncontiguous_vector,
                       void *contiguous_vector,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_vector_t* type_c = (s_smpi_mpi_vector_t*)type;
  int i;
  char* contiguous_vector_char = (char*)contiguous_vector;
  char* noncontiguous_vector_char = (char*)noncontiguous_vector;

  for (i = 0; i < type_c->block_count * count; i++) {
      if (type_c->old_type->has_subtype == 0)
        memcpy(contiguous_vector_char,
               noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);
      else
        ((s_smpi_subtype_t*)type_c->old_type->substruct)->serialize( noncontiguous_vector_char,
                                                                     contiguous_vector_char,
                                                                     type_c->block_length,
                                                                     type_c->old_type->substruct);

    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride*smpi_datatype_get_extent(type_c->old_type);
  }
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
void unserialize_vector( const void *contiguous_vector,
                         void *noncontiguous_vector,
                         size_t count,
                         void *type)
{
  s_smpi_mpi_vector_t* type_c = (s_smpi_mpi_vector_t*)type;
  int i;

  char* contiguous_vector_char = (char*)contiguous_vector;
  char* noncontiguous_vector_char = (char*)noncontiguous_vector;

  for (i = 0; i < type_c->block_count * count; i++) {
    if (type_c->old_type->has_subtype == 0)
      memcpy(noncontiguous_vector_char,
             contiguous_vector_char, type_c->block_length * type_c->size_oldtype);
    else
      ((s_smpi_subtype_t*)type_c->old_type->substruct)->unserialize( contiguous_vector_char,
                                                                     noncontiguous_vector_char,
                                                                     type_c->block_length,
                                                                     type_c->old_type->substruct);
    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride*smpi_datatype_get_extent(type_c->old_type);
  }
}

/*
 * Create a Sub type vector to be able to serialize and unserialize it
 * the structure s_smpi_mpi_vector_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 *
 */
s_smpi_mpi_vector_t* smpi_datatype_vector_create( int block_stride,
                                                  int block_length,
                                                  int block_count,
                                                  MPI_Datatype old_type,
                                                  int size_oldtype){
  s_smpi_mpi_vector_t *new_t= xbt_new(s_smpi_mpi_vector_t,1);
  new_t->base.serialize = &serialize_vector;
  new_t->base.unserialize = &unserialize_vector;
  new_t->base.subtype_free = &free_vector;
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

void smpi_datatype_create(MPI_Datatype* new_type, int size,int lb, int ub, int has_subtype,
                          void *struct_type, int flags){
  MPI_Datatype new_t= xbt_new(s_smpi_mpi_datatype_t,1);
  new_t->size = size;
  new_t->has_subtype = has_subtype;
  new_t->lb = lb;
  new_t->ub = ub;
  new_t->flags = flags;
  new_t->substruct = struct_type;
  new_t->in_use=0;
  *new_type = new_t;
}

void smpi_datatype_free(MPI_Datatype* type){

  if((*type)->flags & DT_FLAG_PREDEFINED)return;

  //if still used, mark for deletion
  if((*type)->in_use!=0){
      (*type)->flags |=DT_FLAG_DESTROYED;
      return;
  }

  if ((*type)->has_subtype == 1){
    ((s_smpi_subtype_t *)(*type)->substruct)->subtype_free(type);  
    xbt_free((*type)->substruct);
  }
  xbt_free(*type);

}

void smpi_datatype_use(MPI_Datatype type){
  if(type)type->in_use++;
}


void smpi_datatype_unuse(MPI_Datatype type){
  if(type && type->in_use-- == 0 && (type->flags & DT_FLAG_DESTROYED))
    smpi_datatype_free(&type);
}




/*
Contiguous Implementation
*/


/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hvector - output hvector
 *  @param noncontiguous_hvector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_contiguous( const void *noncontiguous_hvector,
                       void *contiguous_hvector,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_contiguous_t* type_c = (s_smpi_mpi_contiguous_t*)type;
  char* contiguous_vector_char = (char*)contiguous_hvector;
  char* noncontiguous_vector_char = (char*)noncontiguous_hvector+type_c->lb;
  memcpy(contiguous_vector_char,
           noncontiguous_vector_char, count* type_c->block_count * type_c->size_oldtype);
}
/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_vector - output hvector
 *  @param contiguous_vector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_contiguous( const void *contiguous_vector,
                         void *noncontiguous_vector,
                         size_t count,
                         void *type)
{
  s_smpi_mpi_contiguous_t* type_c = (s_smpi_mpi_contiguous_t*)type;
  char* contiguous_vector_char = (char*)contiguous_vector;
  char* noncontiguous_vector_char = (char*)noncontiguous_vector+type_c->lb;

  memcpy(noncontiguous_vector_char,
           contiguous_vector_char, count*  type_c->block_count * type_c->size_oldtype);
}

void free_contiguous(MPI_Datatype* d){
}

/*
 * Create a Sub type contiguous to be able to serialize and unserialize it
 * the structure s_smpi_mpi_contiguous_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 *
 */
s_smpi_mpi_contiguous_t* smpi_datatype_contiguous_create( MPI_Aint lb,
                                                  int block_count,
                                                  MPI_Datatype old_type,
                                                  int size_oldtype){
  s_smpi_mpi_contiguous_t *new_t= xbt_new(s_smpi_mpi_contiguous_t,1);
  new_t->base.serialize = &serialize_contiguous;
  new_t->base.unserialize = &unserialize_contiguous;
  new_t->base.subtype_free = &free_contiguous;
  new_t->lb = lb;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}




int smpi_datatype_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type, MPI_Aint lb)
{
  int retval;
  if(old_type->has_subtype){
	  //handle this case as a hvector with stride equals to the extent of the datatype
	  return smpi_datatype_hvector(count, 1, smpi_datatype_get_extent(old_type), old_type, new_type);
  }
  
  s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb,
                                                                count,
                                                                old_type,
                                                                smpi_datatype_size(old_type));
                                                                
  smpi_datatype_create(new_type,
					  count * smpi_datatype_size(old_type),
					  lb,lb + count * smpi_datatype_size(old_type),
					  1,subtype, DT_FLAG_CONTIGUOUS);
  retval=MPI_SUCCESS;
  return retval;
}

int smpi_datatype_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (blocklen<0) return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=smpi_datatype_lb(old_type);
    ub=((count-1)*stride+blocklen-1)*smpi_datatype_get_extent(old_type)+smpi_datatype_ub(old_type);
  }
  if(old_type->has_subtype || stride != blocklen){


    s_smpi_mpi_vector_t* subtype = smpi_datatype_vector_create( stride,
                                                                blocklen,
                                                                count,
                                                                old_type,
                                                                smpi_datatype_size(old_type));
    smpi_datatype_create(new_type,
                         count * (blocklen) * smpi_datatype_size(old_type), lb,
                         ub,
                         1,
                         subtype,
                         DT_FLAG_VECTOR);
    retval=MPI_SUCCESS;
  }else{
    /* in this situation the data are contignous thus it's not
     * required to serialize and unserialize it*/
    smpi_datatype_create(new_type, count * blocklen *
                         smpi_datatype_size(old_type), 0, ((count -1) * stride + blocklen)*
                         smpi_datatype_size(old_type),
                         0,
                         NULL,
                         DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
  }
  return retval;
}

void free_vector(MPI_Datatype* d){
}

/*
Hvector Implementation - Vector with stride in bytes
*/


/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hvector - output hvector
 *  @param noncontiguous_hvector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_hvector( const void *noncontiguous_hvector,
                       void *contiguous_hvector,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_hvector_t* type_c = (s_smpi_mpi_hvector_t*)type;
  int i;
  char* contiguous_vector_char = (char*)contiguous_hvector;
  char* noncontiguous_vector_char = (char*)noncontiguous_hvector;

  for (i = 0; i < type_c->block_count * count; i++) {
    if (type_c->old_type->has_subtype == 0)
      memcpy(contiguous_vector_char,
           noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);
    else
      ((s_smpi_subtype_t*)type_c->old_type->substruct)->serialize( noncontiguous_vector_char,
                                                                   contiguous_vector_char,
                                                                   type_c->block_length,
                                                                   type_c->old_type->substruct);

    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride;
  }
}
/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_vector - output hvector
 *  @param contiguous_vector - input hvector
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data, in bytes
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_hvector( const void *contiguous_vector,
                         void *noncontiguous_vector,
                         size_t count,
                         void *type)
{
  s_smpi_mpi_hvector_t* type_c = (s_smpi_mpi_hvector_t*)type;
  int i;

  char* contiguous_vector_char = (char*)contiguous_vector;
  char* noncontiguous_vector_char = (char*)noncontiguous_vector;

  for (i = 0; i < type_c->block_count * count; i++) {
    if (type_c->old_type->has_subtype == 0)
      memcpy(noncontiguous_vector_char,
           contiguous_vector_char, type_c->block_length * type_c->size_oldtype);
    else
      ((s_smpi_subtype_t*)type_c->old_type->substruct)->unserialize( contiguous_vector_char,
                                                                     noncontiguous_vector_char,
                                                                     type_c->block_length,
                                                                     type_c->old_type->substruct);
    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride;
  }
}

/*
 * Create a Sub type vector to be able to serialize and unserialize it
 * the structure s_smpi_mpi_vector_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 *
 */
s_smpi_mpi_hvector_t* smpi_datatype_hvector_create( MPI_Aint block_stride,
                                                  int block_length,
                                                  int block_count,
                                                  MPI_Datatype old_type,
                                                  int size_oldtype){
  s_smpi_mpi_hvector_t *new_t= xbt_new(s_smpi_mpi_hvector_t,1);
  new_t->base.serialize = &serialize_hvector;
  new_t->base.unserialize = &unserialize_hvector;
  new_t->base.subtype_free = &free_hvector;
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

//do nothing for vector types
void free_hvector(MPI_Datatype* d){
}

int smpi_datatype_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (blocklen<0) return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=smpi_datatype_lb(old_type);
    ub=((count-1)*stride)+(blocklen-1)*smpi_datatype_get_extent(old_type)+smpi_datatype_ub(old_type);
  }
  if(old_type->has_subtype || stride != blocklen*smpi_datatype_get_extent(old_type)){
    s_smpi_mpi_hvector_t* subtype = smpi_datatype_hvector_create( stride,
                                                                  blocklen,
                                                                  count,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));

    smpi_datatype_create(new_type, count * blocklen * smpi_datatype_size(old_type),
						 lb,ub,
                         1,
                         subtype,
                         DT_FLAG_VECTOR);
    retval=MPI_SUCCESS;
  }else{
    smpi_datatype_create(new_type, count * blocklen *
                                             smpi_datatype_size(old_type),0,count * blocklen *
                                             smpi_datatype_size(old_type),
                                            0,
                                            NULL,
                                            DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
  }
  return retval;
}


/*
Indexed Implementation
*/

/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_indexed - output indexed
 *  @param noncontiguous_indexed - input indexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in element
 *      - count - the number of rows of matrix
 */
void serialize_indexed( const void *noncontiguous_indexed,
                       void *contiguous_indexed,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_indexed_t* type_c = (s_smpi_mpi_indexed_t*)type;
  int i,j;
  char* contiguous_indexed_char = (char*)contiguous_indexed;
  char* noncontiguous_indexed_char = (char*)noncontiguous_indexed+type_c->block_indices[0] * type_c->size_oldtype;
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_type->has_subtype == 0)
        memcpy(contiguous_indexed_char,
                     noncontiguous_indexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
      else
        ((s_smpi_subtype_t*)type_c->old_type->substruct)->serialize( noncontiguous_indexed_char,
                                                                     contiguous_indexed_char,
                                                                     type_c->block_lengths[i],
                                                                     type_c->old_type->substruct);


      contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
      if (i<type_c->block_count-1)noncontiguous_indexed_char = (char*)noncontiguous_indexed + type_c->block_indices[i+1]*smpi_datatype_get_extent(type_c->old_type);
      else noncontiguous_indexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
    }
    noncontiguous_indexed=(void*)noncontiguous_indexed_char;
  }
}
/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_indexed - output indexed
 *  @param contiguous_indexed - input indexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in element
 *      - count - the number of rows of matrix
 */
void unserialize_indexed( const void *contiguous_indexed,
                         void *noncontiguous_indexed,
                         size_t count,
                         void *type)
{

  s_smpi_mpi_indexed_t* type_c = (s_smpi_mpi_indexed_t*)type;
  int i,j;
  char* contiguous_indexed_char = (char*)contiguous_indexed;
  char* noncontiguous_indexed_char = (char*)noncontiguous_indexed+type_c->block_indices[0]*smpi_datatype_get_extent(type_c->old_type);
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_type->has_subtype == 0)
        memcpy(noncontiguous_indexed_char ,
             contiguous_indexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
      else
        ((s_smpi_subtype_t*)type_c->old_type->substruct)->unserialize( contiguous_indexed_char,
                                                                       noncontiguous_indexed_char,
                                                                       type_c->block_lengths[i],
                                                                       type_c->old_type->substruct);

      contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
      if (i<type_c->block_count-1)
        noncontiguous_indexed_char = (char*)noncontiguous_indexed + type_c->block_indices[i+1]*smpi_datatype_get_extent(type_c->old_type);
      else noncontiguous_indexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
    }
    noncontiguous_indexed=(void*)noncontiguous_indexed_char;
  }
}

void free_indexed(MPI_Datatype* type){
  xbt_free(((s_smpi_mpi_indexed_t *)(*type)->substruct)->block_lengths);
  xbt_free(((s_smpi_mpi_indexed_t *)(*type)->substruct)->block_indices);
}

/*
 * Create a Sub type indexed to be able to serialize and unserialize it
 * the structure s_smpi_mpi_indexed_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 */
s_smpi_mpi_indexed_t* smpi_datatype_indexed_create( int* block_lengths,
                                                  int* block_indices,
                                                  int block_count,
                                                  MPI_Datatype old_type,
                                                  int size_oldtype){
  s_smpi_mpi_indexed_t *new_t= xbt_new(s_smpi_mpi_indexed_t,1);
  new_t->base.serialize = &serialize_indexed;
  new_t->base.unserialize = &unserialize_indexed;
  new_t->base.subtype_free = &free_indexed;
 //TODO : add a custom function for each time to clean these 
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(int, block_count);
  int i;
  for(i=0;i<block_count;i++){
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
  }
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}


int smpi_datatype_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int i;
  int retval;
  int size = 0;
  int contiguous=1;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0]*smpi_datatype_get_extent(old_type);
    ub=indices[0]*smpi_datatype_get_extent(old_type) + blocklens[0]*smpi_datatype_ub(old_type);
  }

  for(i=0; i< count; i++){
    if   (blocklens[i]<0)
      return MPI_ERR_ARG;
    size += blocklens[i];

    if(indices[i]*smpi_datatype_get_extent(old_type)+smpi_datatype_lb(old_type)<lb)
    	lb = indices[i]*smpi_datatype_get_extent(old_type)+smpi_datatype_lb(old_type);
    if(indices[i]*smpi_datatype_get_extent(old_type)+blocklens[i]*smpi_datatype_ub(old_type)>ub)
    	ub = indices[i]*smpi_datatype_get_extent(old_type)+blocklens[i]*smpi_datatype_ub(old_type);

    if ( (i< count -1) && (indices[i]+blocklens[i] != indices[i+1]) )contiguous=0;
  }
  if (old_type->has_subtype == 1)
    contiguous=0;

  if(!contiguous){
    s_smpi_mpi_indexed_t* subtype = smpi_datatype_indexed_create( blocklens,
                                                                  indices,
                                                                  count,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));
     smpi_datatype_create(new_type,  size *
                         smpi_datatype_size(old_type),lb,ub,1, subtype, DT_FLAG_DATA);
  }else{
    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb,
                                                                  size,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));
    smpi_datatype_create(new_type,  size *
                         smpi_datatype_size(old_type),lb,ub,1, subtype, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
  }
  retval=MPI_SUCCESS;
  return retval;
}


/*
Hindexed Implementation - Indexed with indices in bytes 
*/

/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_hindexed - output hindexed
 *  @param noncontiguous_hindexed - input hindexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in bytes
 *      - count - the number of rows of matrix
 */
void serialize_hindexed( const void *noncontiguous_hindexed,
                       void *contiguous_hindexed,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_hindexed_t* type_c = (s_smpi_mpi_hindexed_t*)type;
  int i,j;
  char* contiguous_hindexed_char = (char*)contiguous_hindexed;
  char* noncontiguous_hindexed_char = (char*)noncontiguous_hindexed+ type_c->block_indices[0];
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_type->has_subtype == 0)
        memcpy(contiguous_hindexed_char,
                     noncontiguous_hindexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
      else
        ((s_smpi_subtype_t*)type_c->old_type->substruct)->serialize( noncontiguous_hindexed_char,
                                                                     contiguous_hindexed_char,
                                                                     type_c->block_lengths[i],
                                                                     type_c->old_type->substruct);

      contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
      if (i<type_c->block_count-1)noncontiguous_hindexed_char = (char*)noncontiguous_hindexed + type_c->block_indices[i+1];
      else noncontiguous_hindexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
    }
    noncontiguous_hindexed=(void*)noncontiguous_hindexed_char;
  }
}
/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_hindexed - output hindexed
 *  @param contiguous_hindexed - input hindexed
 *  @param type - pointer contening :
 *      - block_lengths - the width or height of blocked matrix
 *      - block_indices - indices of each data, in bytes
 *      - count - the number of rows of matrix
 */
void unserialize_hindexed( const void *contiguous_hindexed,
                         void *noncontiguous_hindexed,
                         size_t count,
                         void *type)
{
  s_smpi_mpi_hindexed_t* type_c = (s_smpi_mpi_hindexed_t*)type;
  int i,j;

  char* contiguous_hindexed_char = (char*)contiguous_hindexed;
  char* noncontiguous_hindexed_char = (char*)noncontiguous_hindexed+ type_c->block_indices[0];
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_type->has_subtype == 0)
        memcpy(noncontiguous_hindexed_char,
               contiguous_hindexed_char, type_c->block_lengths[i] * type_c->size_oldtype);
      else
        ((s_smpi_subtype_t*)type_c->old_type->substruct)->unserialize( contiguous_hindexed_char,
                                                                       noncontiguous_hindexed_char,
                                                                       type_c->block_lengths[i],
                                                                       type_c->old_type->substruct);

      contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
      if (i<type_c->block_count-1)noncontiguous_hindexed_char = (char*)noncontiguous_hindexed + type_c->block_indices[i+1];
      else noncontiguous_hindexed_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_type);
    }
    noncontiguous_hindexed=(void*)noncontiguous_hindexed_char;
  }
}

void free_hindexed(MPI_Datatype* type){
  xbt_free(((s_smpi_mpi_hindexed_t *)(*type)->substruct)->block_lengths);
  xbt_free(((s_smpi_mpi_hindexed_t *)(*type)->substruct)->block_indices);
}

/*
 * Create a Sub type hindexed to be able to serialize and unserialize it
 * the structure s_smpi_mpi_hindexed_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 */
s_smpi_mpi_hindexed_t* smpi_datatype_hindexed_create( int* block_lengths,
                                                  MPI_Aint* block_indices,
                                                  int block_count,
                                                  MPI_Datatype old_type,
                                                  int size_oldtype){
  s_smpi_mpi_hindexed_t *new_t= xbt_new(s_smpi_mpi_hindexed_t,1);
  new_t->base.serialize = &serialize_hindexed;
  new_t->base.unserialize = &unserialize_hindexed;
  new_t->base.subtype_free = &free_hindexed;
 //TODO : add a custom function for each time to clean these 
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(MPI_Aint, block_count);
  int i;
  for(i=0;i<block_count;i++){
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
  }
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}


int smpi_datatype_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int i;
  int retval;
  int size = 0;
  int contiguous=1;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + smpi_datatype_lb(old_type);
    ub=indices[0] + blocklens[0]*smpi_datatype_ub(old_type);
  }
  for(i=0; i< count; i++){
    if   (blocklens[i]<0)
      return MPI_ERR_ARG;
    size += blocklens[i];

    if(indices[i]+smpi_datatype_lb(old_type)<lb) lb = indices[i]+smpi_datatype_lb(old_type);
    if(indices[i]+blocklens[i]*smpi_datatype_ub(old_type)>ub) ub = indices[i]+blocklens[i]*smpi_datatype_ub(old_type);

    if ( (i< count -1) && (indices[i]+blocklens[i]*smpi_datatype_size(old_type) != indices[i+1]) )contiguous=0;
  }
  if (old_type->has_subtype == 1 || lb!=0)
    contiguous=0;

  if(!contiguous){
    s_smpi_mpi_hindexed_t* subtype = smpi_datatype_hindexed_create( blocklens,
                                                                  indices,
                                                                  count,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));
    smpi_datatype_create(new_type,  size * smpi_datatype_size(old_type),
						 lb,
                         ub
                         ,1, subtype, DT_FLAG_DATA);
  }else{
    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb,
                                                                  size,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));
    smpi_datatype_create(new_type,  size * smpi_datatype_size(old_type),
					     0,size * smpi_datatype_size(old_type),
					     1, subtype, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
  }
  retval=MPI_SUCCESS;
  return retval;
}


/*
struct Implementation - Indexed with indices in bytes 
*/

/*
 *  Copies noncontiguous data into contiguous memory.
 *  @param contiguous_struct - output struct
 *  @param noncontiguous_struct - input struct
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void serialize_struct( const void *noncontiguous_struct,
                       void *contiguous_struct,
                       size_t count,
                       void *type)
{
  s_smpi_mpi_struct_t* type_c = (s_smpi_mpi_struct_t*)type;
  int i,j;
  char* contiguous_struct_char = (char*)contiguous_struct;
  char* noncontiguous_struct_char = (char*)noncontiguous_struct+ type_c->block_indices[0];
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_types[i]->has_subtype == 0)
        memcpy(contiguous_struct_char,
             noncontiguous_struct_char, type_c->block_lengths[i] * smpi_datatype_size(type_c->old_types[i]));
      else
        ((s_smpi_subtype_t*)type_c->old_types[i]->substruct)->serialize( noncontiguous_struct_char,
                                                                         contiguous_struct_char,
                                                                         type_c->block_lengths[i],
                                                                         type_c->old_types[i]->substruct);


      contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
      if (i<type_c->block_count-1)noncontiguous_struct_char = (char*)noncontiguous_struct + type_c->block_indices[i+1];
      else noncontiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_types[i]);//let's hope this is MPI_UB ?
    }
    noncontiguous_struct=(void*)noncontiguous_struct_char;
  }
}
/*
 *  Copies contiguous data into noncontiguous memory.
 *  @param noncontiguous_struct - output struct
 *  @param contiguous_struct - input struct
 *  @param type - pointer contening :
 *      - stride - stride of between noncontiguous data
 *      - block_length - the width or height of blocked matrix
 *      - count - the number of rows of matrix
 */
void unserialize_struct( const void *contiguous_struct,
                         void *noncontiguous_struct,
                         size_t count,
                         void *type)
{
  s_smpi_mpi_struct_t* type_c = (s_smpi_mpi_struct_t*)type;
  int i,j;

  char* contiguous_struct_char = (char*)contiguous_struct;
  char* noncontiguous_struct_char = (char*)noncontiguous_struct+ type_c->block_indices[0];
  for(j=0; j<count;j++){
    for (i = 0; i < type_c->block_count; i++) {
      if (type_c->old_types[i]->has_subtype == 0)
        memcpy(noncontiguous_struct_char,
             contiguous_struct_char, type_c->block_lengths[i] * smpi_datatype_size(type_c->old_types[i]));
      else
        ((s_smpi_subtype_t*)type_c->old_types[i]->substruct)->unserialize( contiguous_struct_char,
                                                                           noncontiguous_struct_char,
                                                                           type_c->block_lengths[i],
                                                                           type_c->old_types[i]->substruct);

      contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
      if (i<type_c->block_count-1)noncontiguous_struct_char =  (char*)noncontiguous_struct + type_c->block_indices[i+1];
      else noncontiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_get_extent(type_c->old_types[i]);
    }
    noncontiguous_struct=(void*)noncontiguous_struct_char;
    
  }
}

void free_struct(MPI_Datatype* type){
  xbt_free(((s_smpi_mpi_struct_t *)(*type)->substruct)->block_lengths);
  xbt_free(((s_smpi_mpi_struct_t *)(*type)->substruct)->block_indices);
  xbt_free(((s_smpi_mpi_struct_t *)(*type)->substruct)->old_types);
}

/*
 * Create a Sub type struct to be able to serialize and unserialize it
 * the structure s_smpi_mpi_struct_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 */
s_smpi_mpi_struct_t* smpi_datatype_struct_create( int* block_lengths,
                                                  MPI_Aint* block_indices,
                                                  int block_count,
                                                  MPI_Datatype* old_types){
  s_smpi_mpi_struct_t *new_t= xbt_new(s_smpi_mpi_struct_t,1);
  new_t->base.serialize = &serialize_struct;
  new_t->base.unserialize = &unserialize_struct;
  new_t->base.subtype_free = &free_struct;
 //TODO : add a custom function for each time to clean these 
  new_t->block_lengths= xbt_new(int, block_count);
  new_t->block_indices= xbt_new(MPI_Aint, block_count);
  new_t->old_types=  xbt_new(MPI_Datatype, block_count);
  int i;
  for(i=0;i<block_count;i++){
    new_t->block_lengths[i]=block_lengths[i];
    new_t->block_indices[i]=block_indices[i];
    new_t->old_types[i]=old_types[i];
  }
  //new_t->block_lengths = block_lengths;
  //new_t->block_indices = block_indices;
  new_t->block_count = block_count;
  //new_t->old_types = old_types;
  return new_t;
}


int smpi_datatype_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type)
{
  int i;
  size_t size = 0;
  int contiguous=1;
  size = 0;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + smpi_datatype_lb(old_types[0]);
    ub=indices[0] + blocklens[0]*smpi_datatype_ub(old_types[0]);
  }
  int forced_lb=0;
  int forced_ub=0;
  for(i=0; i< count; i++){
    if (blocklens[i]<0)
      return MPI_ERR_ARG;
    if (old_types[i]->has_subtype == 1)
      contiguous=0;

    size += blocklens[i]*smpi_datatype_size(old_types[i]);
    if (old_types[i]==MPI_LB){
      lb=indices[i];
      forced_lb=1;
    }
    if (old_types[i]==MPI_UB){
      ub=indices[i];
      forced_ub=1;
    }

    if(!forced_lb && indices[i]+smpi_datatype_lb(old_types[i])<lb) lb = indices[i];
    if(!forced_ub && indices[i]+blocklens[i]*smpi_datatype_ub(old_types[i])>ub) ub = indices[i]+blocklens[i]*smpi_datatype_ub(old_types[i]);

    if ( (i< count -1) && (indices[i]+blocklens[i]*smpi_datatype_size(old_types[i]) != indices[i+1]) )contiguous=0;
  }

  if(!contiguous){
    s_smpi_mpi_struct_t* subtype = smpi_datatype_struct_create( blocklens,
                                                              indices,
                                                              count,
                                                              old_types);

    smpi_datatype_create(new_type,  size, lb, ub,1, subtype, DT_FLAG_DATA);
  }else{
    s_smpi_mpi_contiguous_t* subtype = smpi_datatype_contiguous_create( lb,
                                                                  size,
                                                                  MPI_CHAR,
                                                                  1);
    smpi_datatype_create(new_type,  size, lb, ub,1, subtype, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
  }
  return MPI_SUCCESS;
}

void smpi_datatype_commit(MPI_Datatype *datatype)
{
  (*datatype)->flags=  ((*datatype)->flags | DT_FLAG_COMMITED);
}

typedef struct s_smpi_mpi_op {
  MPI_User_function *func;
  int is_commute;
} s_smpi_mpi_op_t;

#define MAX_OP(a, b)  (b) = (a) < (b) ? (b) : (a)
#define MIN_OP(a, b)  (b) = (a) < (b) ? (a) : (b)
#define SUM_OP(a, b)  (b) += (a)
#define PROD_OP(a, b) (b) *= (a)
#define LAND_OP(a, b) (b) = (a) && (b)
#define LOR_OP(a, b)  (b) = (a) || (b)
#define LXOR_OP(a, b) (b) = (!(a) && (b)) || ((a) && !(b))
#define BAND_OP(a, b) (b) &= (a)
#define BOR_OP(a, b)  (b) |= (a)
#define BXOR_OP(a, b) (b) ^= (a)
#define MAXLOC_OP(a, b)  (b) = (a.value) < (b.value) ? (b) : (a)
#define MINLOC_OP(a, b)  (b) = (a.value) < (b.value) ? (a) : (b)
//TODO : MINLOC & MAXLOC

#define APPLY_FUNC(a, b, length, type, func) \
{                                          \
  int i;                                   \
  type* x = (type*)(a);                    \
  type* y = (type*)(b);                    \
  for(i = 0; i < *(length); i++) {         \
    func(x[i], y[i]);                      \
  }                                        \
}

static void max_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, MAX_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, MAX_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, MAX_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, MAX_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, MAX_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, MAX_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, MAX_OP);
  }
}

static void min_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, MIN_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, MIN_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, MIN_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, MIN_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, MIN_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, MIN_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, MIN_OP);
  }
}

static void sum_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, SUM_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, SUM_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, SUM_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, SUM_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, SUM_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, SUM_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, SUM_OP);
  } else if (*datatype == MPI_C_FLOAT_COMPLEX) {
    APPLY_FUNC(a, b, length, float _Complex, SUM_OP);
  } else if (*datatype == MPI_C_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, double _Complex, SUM_OP);
  } else if (*datatype == MPI_C_LONG_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, long double _Complex, SUM_OP);
  }
}

static void prod_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, PROD_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, PROD_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, PROD_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, PROD_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, PROD_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, PROD_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, PROD_OP);
  } else if (*datatype == MPI_C_FLOAT_COMPLEX) {
    APPLY_FUNC(a, b, length, float _Complex, PROD_OP);
  } else if (*datatype == MPI_C_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, double _Complex, PROD_OP);
  } else if (*datatype == MPI_C_LONG_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, long double _Complex, PROD_OP);
  }
}

static void land_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LAND_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LAND_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LAND_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LAND_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LAND_OP);
  }
}

static void lor_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LOR_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LOR_OP);
  }
}

static void lxor_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LXOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LXOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LXOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LXOR_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LXOR_OP);
  }
}

static void band_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BAND_OP);
  }
  if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BAND_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BAND_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BAND_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BAND_OP);
  }
}

static void bor_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BOR_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BOR_OP);
  }
}

static void bxor_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BXOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BXOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BXOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BXOR_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BXOR_OP);
  }
}

static void minloc_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype)
{
  if (*datatype == MPI_FLOAT_INT) {
    APPLY_FUNC(a, b, length, float_int, MINLOC_OP);
  } else if (*datatype == MPI_LONG_INT) {
    APPLY_FUNC(a, b, length, long_int, MINLOC_OP);
  } else if (*datatype == MPI_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, double_int, MINLOC_OP);
  } else if (*datatype == MPI_SHORT_INT) {
    APPLY_FUNC(a, b, length, short_int, MINLOC_OP);
  } else if (*datatype == MPI_2INT) {
    APPLY_FUNC(a, b, length, int_int, MINLOC_OP);
  } else if (*datatype == MPI_LONG_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, long_double_int, MINLOC_OP);
  } else if (*datatype == MPI_2FLOAT) {
    APPLY_FUNC(a, b, length, float_float, MINLOC_OP);
  } else if (*datatype == MPI_2DOUBLE) {
    APPLY_FUNC(a, b, length, double_double, MINLOC_OP);
  }
}

static void maxloc_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype)
{
  if (*datatype == MPI_FLOAT_INT) {
    APPLY_FUNC(a, b, length, float_int, MAXLOC_OP);
  } else if (*datatype == MPI_LONG_INT) {
    APPLY_FUNC(a, b, length, long_int, MAXLOC_OP);
  } else if (*datatype == MPI_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, double_int, MAXLOC_OP);
  } else if (*datatype == MPI_SHORT_INT) {
    APPLY_FUNC(a, b, length, short_int, MAXLOC_OP);
  } else if (*datatype == MPI_2INT) {
    APPLY_FUNC(a, b, length, int_int, MAXLOC_OP);
  } else if (*datatype == MPI_LONG_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, long_double_int, MAXLOC_OP);
  } else if (*datatype == MPI_2FLOAT) {
    APPLY_FUNC(a, b, length, float_float, MAXLOC_OP);
  } else if (*datatype == MPI_2DOUBLE) {
    APPLY_FUNC(a, b, length, double_double, MAXLOC_OP);
  }
}


#define CREATE_MPI_OP(name, func)                             \
  static s_smpi_mpi_op_t mpi_##name = { &(func) /* func */, TRUE }; \
MPI_Op name = &mpi_##name;

CREATE_MPI_OP(MPI_MAX, max_func);
CREATE_MPI_OP(MPI_MIN, min_func);
CREATE_MPI_OP(MPI_SUM, sum_func);
CREATE_MPI_OP(MPI_PROD, prod_func);
CREATE_MPI_OP(MPI_LAND, land_func);
CREATE_MPI_OP(MPI_LOR, lor_func);
CREATE_MPI_OP(MPI_LXOR, lxor_func);
CREATE_MPI_OP(MPI_BAND, band_func);
CREATE_MPI_OP(MPI_BOR, bor_func);
CREATE_MPI_OP(MPI_BXOR, bxor_func);
CREATE_MPI_OP(MPI_MAXLOC, maxloc_func);
CREATE_MPI_OP(MPI_MINLOC, minloc_func);

MPI_Op smpi_op_new(MPI_User_function * function, int commute)
{
  MPI_Op op;
  op = xbt_new(s_smpi_mpi_op_t, 1);
  op->func = function;
  op-> is_commute = commute;
  return op;
}

int smpi_op_is_commute(MPI_Op op)
{
  return op-> is_commute;
}

void smpi_op_destroy(MPI_Op op)
{
  xbt_free(op);
}

void smpi_op_apply(MPI_Op op, void *invec, void *inoutvec, int *len,
                   MPI_Datatype * datatype)
{
  op->func(invec, inoutvec, len, datatype);
}
