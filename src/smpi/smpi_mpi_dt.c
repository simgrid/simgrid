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


//The following are datatypes for the MPI functions MPI_MAXLOC and MPI_MINLOC.
typedef struct {
  float value;
  int index;
} float_int;
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
CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE_INT, long_double_int);

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
  int retval;

  if ((datatype->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
    *lb = datatype->lb;
    *extent = datatype->ub - datatype->lb;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
  int retval, count;

  /* First check if we really have something to do */
  if (recvcount == 0) {
    retval = sendcount == 0 ? MPI_SUCCESS : MPI_ERR_TRUNCATE;
  } else {
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

      s_smpi_mpi_vector_t* type_c = (s_smpi_mpi_vector_t*)sendtype;

      void * buf_tmp = malloc(count * type_c->size_oldtype);

      subtype->serialize( sendbuf, buf_tmp,1, subtype);
      subtype =  recvtype->substruct;
      subtype->unserialize(recvbuf, buf_tmp,1, subtype);

      free(buf_tmp);
    }
    retval = sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
  }

  return retval;
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
    memcpy(contiguous_vector_char,
           noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);

    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride*type_c->size_oldtype;
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
    memcpy(noncontiguous_vector_char,
           contiguous_vector_char, type_c->block_length * type_c->size_oldtype);

    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride*type_c->size_oldtype;
  }
}

/*
 * Create a Sub type vector to be able to serialize and unserialize it
 * the structre s_smpi_mpi_vector_t is derived from s_smpi_subtype which
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
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

void smpi_datatype_create(MPI_Datatype* new_type, int size, int has_subtype,
                          void *struct_type, int flags){
  MPI_Datatype new_t= xbt_new(s_smpi_mpi_datatype_t,1);
  new_t->size = size;
  new_t->has_subtype = has_subtype;
  new_t->lb = 0;
  new_t->ub = size;
  new_t->flags = flags;
  new_t->substruct = struct_type;
  *new_type = new_t;
}

void smpi_datatype_free(MPI_Datatype* type){
  xbt_free(*type);
}

int smpi_datatype_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if ((old_type->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_datatype_create(new_type, count *
                         smpi_datatype_size(old_type),1,NULL, DT_FLAG_CONTIGUOUS);
    retval=MPI_SUCCESS;
  }
  return retval;
}

int smpi_datatype_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (blocklen<=0) return MPI_ERR_ARG;
  if ((old_type->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
    if(stride != blocklen){
if (old_type->has_subtype == 1)
      XBT_WARN("vector contains a complex type - not yet handled");
      s_smpi_mpi_vector_t* subtype = smpi_datatype_vector_create( stride,
                                                                  blocklen,
                                                                  count,
                                                                  old_type,
                                                                  smpi_datatype_size(old_type));

      smpi_datatype_create(new_type, count * (blocklen) *
                           smpi_datatype_size(old_type),
                           1,
                           subtype,
                           DT_FLAG_VECTOR);
      retval=MPI_SUCCESS;
    }else{
      /* in this situation the data are contignous thus it's not
       * required to serialize and unserialize it*/
      smpi_datatype_create(new_type, count * blocklen *
                           smpi_datatype_size(old_type),
                           0,
                           NULL,
                           DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
      retval=MPI_SUCCESS;
    }
  }
  return retval;
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
    memcpy(contiguous_vector_char,
           noncontiguous_vector_char, type_c->block_length * type_c->size_oldtype);

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
    memcpy(noncontiguous_vector_char,
           contiguous_vector_char, type_c->block_length * type_c->size_oldtype);

    contiguous_vector_char += type_c->block_length*type_c->size_oldtype;
    noncontiguous_vector_char += type_c->block_stride;
  }
}

/*
 * Create a Sub type vector to be able to serialize and unserialize it
 * the structre s_smpi_mpi_vector_t is derived from s_smpi_subtype which
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
  new_t->block_stride = block_stride;
  new_t->block_length = block_length;
  new_t->block_count = block_count;
  new_t->old_type = old_type;
  new_t->size_oldtype = size_oldtype;
  return new_t;
}

int smpi_datatype_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (blocklen<=0) return MPI_ERR_ARG;
  if ((old_type->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
if (old_type->has_subtype == 1)
      XBT_WARN("hvector contains a complex type - not yet handled");
    if(stride != blocklen*smpi_datatype_size(old_type)){
      s_smpi_mpi_hvector_t* subtype = smpi_datatype_hvector_create( stride,
                                                                    blocklen,
                                                                    count,
                                                                    old_type,
                                                                    smpi_datatype_size(old_type));

      smpi_datatype_create(new_type, count * blocklen *
                           smpi_datatype_size(old_type),
                           1,
                           subtype,
                           DT_FLAG_VECTOR);
      retval=MPI_SUCCESS;
    }else{
      smpi_datatype_create(new_type, count * blocklen *
                                               smpi_datatype_size(old_type),
                                              0,
                                              NULL,
                                              DT_FLAG_VECTOR|DT_FLAG_CONTIGUOUS);
      retval=MPI_SUCCESS;
    }
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
  int i;
  char* contiguous_indexed_char = (char*)contiguous_indexed;
  char* noncontiguous_indexed_char = (char*)noncontiguous_indexed;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(contiguous_indexed_char,
           noncontiguous_indexed_char, type_c->block_lengths[i] * type_c->size_oldtype);

    contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
    noncontiguous_indexed_char = (char*)noncontiguous_indexed + type_c->block_indices[i+1]*type_c->size_oldtype;
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
  int i;

  char* contiguous_indexed_char = (char*)contiguous_indexed;
  char* noncontiguous_indexed_char = (char*)noncontiguous_indexed;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(noncontiguous_indexed_char,
           contiguous_indexed_char, type_c->block_lengths[i] * type_c->size_oldtype);

    contiguous_indexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
    noncontiguous_indexed_char = (char*)noncontiguous_indexed + type_c->block_indices[i+1]*type_c->size_oldtype;
  }
}

/*
 * Create a Sub type indexed to be able to serialize and unserialize it
 * the structre s_smpi_mpi_indexed_t is derived from s_smpi_subtype which
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
 //FIXME : copy those or assume they won't be freed ? 
  new_t->block_lengths = block_lengths;
  new_t->block_indices = block_indices;
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
  for(i=0; i< count; i++){
    if   (blocklens[i]<=0)
      return MPI_ERR_ARG;
    size += blocklens[i];

    if ( (i< count -1) && (indices[i]+blocklens[i] != indices[i+1]) )contiguous=0;
  }
  if ((old_type->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {

    if (old_type->has_subtype == 1)
      XBT_WARN("indexed contains a complex type - not yet handled");

    if(!contiguous){
      s_smpi_mpi_indexed_t* subtype = smpi_datatype_indexed_create( blocklens,
                                                                    indices,
                                                                    count,
                                                                    old_type,
                                                                    smpi_datatype_size(old_type));

      smpi_datatype_create(new_type,  size *
                           smpi_datatype_size(old_type),1, subtype, DT_FLAG_DATA);
}else{
      smpi_datatype_create(new_type,  size *
                           smpi_datatype_size(old_type),0, NULL, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
}
    retval=MPI_SUCCESS;
  }
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
  int i;
  char* contiguous_hindexed_char = (char*)contiguous_hindexed;
  char* noncontiguous_hindexed_char = (char*)noncontiguous_hindexed;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(contiguous_hindexed_char,
           noncontiguous_hindexed_char, type_c->block_lengths[i] * type_c->size_oldtype);

    contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
    noncontiguous_hindexed_char = (char*)noncontiguous_hindexed + type_c->block_indices[i+1];
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
  int i;

  char* contiguous_hindexed_char = (char*)contiguous_hindexed;
  char* noncontiguous_hindexed_char = (char*)noncontiguous_hindexed;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(noncontiguous_hindexed_char,
           contiguous_hindexed_char, type_c->block_lengths[i] * type_c->size_oldtype);

    contiguous_hindexed_char += type_c->block_lengths[i]*type_c->size_oldtype;
    noncontiguous_hindexed_char = (char*)noncontiguous_hindexed + type_c->block_indices[i+1];
  }
}

/*
 * Create a Sub type hindexed to be able to serialize and unserialize it
 * the structre s_smpi_mpi_hindexed_t is derived from s_smpi_subtype which
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
 //FIXME : copy those or assume they won't be freed ? 
  new_t->block_lengths = block_lengths;
  new_t->block_indices = block_indices;
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
  for(i=0; i< count; i++){
    if   (blocklens[i]<=0)
      return MPI_ERR_ARG;
    size += blocklens[i];


    if ( (i< count -1) && (indices[i]+blocklens[i]*smpi_datatype_size(old_type) != indices[i+1]) )contiguous=0;
  }
  if ((old_type->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
    if (old_type->has_subtype == 1)
      XBT_WARN("hindexed contains a complex type - not yet handled");

    if(!contiguous){
      s_smpi_mpi_hindexed_t* subtype = smpi_datatype_hindexed_create( blocklens,
                                                                    indices,
                                                                    count,
                                                                    old_type,
                                                                    smpi_datatype_size(old_type));

      smpi_datatype_create(new_type,  size *
                           smpi_datatype_size(old_type),1, subtype, DT_FLAG_DATA);
    }else{
      smpi_datatype_create(new_type,  size *
                           smpi_datatype_size(old_type),0, NULL, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
    }
    retval=MPI_SUCCESS;
  }
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
  int i;
  char* contiguous_struct_char = (char*)contiguous_struct;
  char* noncontiguous_struct_char = (char*)noncontiguous_struct;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(contiguous_struct_char,
           noncontiguous_struct_char, type_c->block_lengths[i] * smpi_datatype_size(type_c->old_types[i]));
    contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
    noncontiguous_struct_char = (char*)noncontiguous_struct + type_c->block_indices[i+1];
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
  int i;

  char* contiguous_struct_char = (char*)contiguous_struct;
  char* noncontiguous_struct_char = (char*)noncontiguous_struct;

  for (i = 0; i < type_c->block_count * count; i++) {
    memcpy(noncontiguous_struct_char,
           contiguous_struct_char, type_c->block_lengths[i] * smpi_datatype_size(type_c->old_types[i]));
    contiguous_struct_char += type_c->block_lengths[i]*smpi_datatype_size(type_c->old_types[i]);
    noncontiguous_struct_char = (char*)noncontiguous_struct + type_c->block_indices[i+1];
  }
}

/*
 * Create a Sub type struct to be able to serialize and unserialize it
 * the structre s_smpi_mpi_struct_t is derived from s_smpi_subtype which
 * required the functions unserialize and serialize
 */
s_smpi_mpi_struct_t* smpi_datatype_struct_create( int* block_lengths,
                                                  MPI_Aint* block_indices,
                                                  int block_count,
                                                  MPI_Datatype* old_types){
  s_smpi_mpi_struct_t *new_t= xbt_new(s_smpi_mpi_struct_t,1);
  new_t->base.serialize = &serialize_struct;
  new_t->base.unserialize = &unserialize_struct;
 //FIXME : copy those or assume they won't be freed ? 
  new_t->block_lengths = block_lengths;
  new_t->block_indices = block_indices;
  new_t->block_count = block_count;
  new_t->old_types = old_types;
  return new_t;
}


int smpi_datatype_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type)
{
  int i;
  size_t size = 0;
  int contiguous=1;
  size = 0;
  for(i=0; i< count; i++){
    if (blocklens[i]<=0)
      return MPI_ERR_ARG;
    if ((old_types[i]->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED)
      return MPI_ERR_TYPE;
    if (old_types[i]->has_subtype == 1)
      XBT_WARN("Struct contains a complex type - not yet handled");
    size += blocklens[i]*smpi_datatype_size(old_types[i]);

    if ( (i< count -1) && (indices[i]+blocklens[i]*smpi_datatype_size(old_types[i]) != indices[i+1]) )contiguous=0;
  }

  if(!contiguous){
    s_smpi_mpi_struct_t* subtype = smpi_datatype_struct_create( blocklens,
                                                              indices,
                                                              count,
                                                              old_types);

    smpi_datatype_create(new_type,  size ,1, subtype, DT_FLAG_DATA);
  }else{
    smpi_datatype_create(new_type,  size,0, NULL, DT_FLAG_DATA|DT_FLAG_CONTIGUOUS);
  }
  return MPI_SUCCESS;
}

void smpi_datatype_commit(MPI_Datatype *datatype)
{
  (*datatype)->flags=  ((*datatype)->flags | DT_FLAG_COMMITED);
}

typedef struct s_smpi_mpi_op {
  MPI_User_function *func;
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
  }
}


#define CREATE_MPI_OP(name, func)                             \
  static s_smpi_mpi_op_t mpi_##name = { &(func) /* func */ }; \
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

  //FIXME: add commute param
  op = xbt_new(s_smpi_mpi_op_t, 1);
  op->func = function;
  return op;
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
