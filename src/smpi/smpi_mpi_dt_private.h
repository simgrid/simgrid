/* smpi_mpi_dt_private.h -- functions of smpi_mpi_dt.c that are exported to other SMPI modules. */

/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_DT_PRIVATE_H
#define SMPI_DT_PRIVATE_H

#include <xbt/base.h>

#include "private.h"

SG_BEGIN_DECL()

#define DT_FLAG_DESTROYED     0x0001  /**< user destroyed but some other layers still have a reference */
#define DT_FLAG_COMMITED      0x0002  /**< ready to be used for a send/recv operation */
#define DT_FLAG_CONTIGUOUS    0x0004  /**< contiguous datatype */
#define DT_FLAG_OVERLAP       0x0008  /**< datatype is unpropper for a recv operation */
#define DT_FLAG_USER_LB       0x0010  /**< has a user defined LB */
#define DT_FLAG_USER_UB       0x0020  /**< has a user defined UB */
#define DT_FLAG_PREDEFINED    0x0040  /**< cannot be removed: initial and predefined datatypes */
#define DT_FLAG_NO_GAPS       0x0080  /**< no gaps around the datatype */
#define DT_FLAG_DATA          0x0100  /**< data or control structure */
#define DT_FLAG_ONE_SIDED     0x0200  /**< datatype can be used for one sided operations */
#define DT_FLAG_UNAVAILABLE   0x0400  /**< datatypes unavailable on the build (OS or compiler dependant) */
#define DT_FLAG_VECTOR        0x0800  /**< valid only for loops. The loop contain only one element
                                       **< without extent. It correspond to the vector type. */
/*
 * We should make the difference here between the predefined contiguous and non contiguous
 * datatypes. The DT_FLAG_BASIC is held by all predefined contiguous datatypes.
 */
#define DT_FLAG_BASIC      (DT_FLAG_PREDEFINED | DT_FLAG_CONTIGUOUS | DT_FLAG_NO_GAPS | DT_FLAG_DATA | DT_FLAG_COMMITED)

extern const MPI_Datatype MPI_PTR;

/* Structures that handle complex data type information, used for serialization/unserialization of messages */
typedef struct s_smpi_mpi_contiguous{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  MPI_Aint lb;
  size_t size_oldtype;
  int block_count;
} s_smpi_mpi_contiguous_t;

typedef struct s_smpi_mpi_vector{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  size_t size_oldtype;
  int block_stride;
  int block_length;
  int block_count;
} s_smpi_mpi_vector_t;

typedef struct s_smpi_mpi_hvector{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  size_t size_oldtype;
  MPI_Aint block_stride;
  int block_length;
  int block_count;
} s_smpi_mpi_hvector_t;

typedef struct s_smpi_mpi_indexed{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  size_t size_oldtype;
  int* block_lengths;
  int* block_indices;
  int block_count;
} s_smpi_mpi_indexed_t;

typedef struct s_smpi_mpi_hindexed{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  size_t size_oldtype;
  int* block_lengths;
  MPI_Aint* block_indices;
  int block_count;
} s_smpi_mpi_hindexed_t;

typedef struct s_smpi_mpi_struct{
  s_smpi_subtype_t base;
  MPI_Datatype old_type;
  size_t size_oldtype;
  int* block_lengths;
  MPI_Aint* block_indices;
  int block_count;
  MPI_Datatype* old_types;
} s_smpi_mpi_struct_t;

/*
  Functions to handle serialization/unserialization of messages, 3 for each type of MPI_Type
  One for creating the substructure to handle, one for serialization, one for unserialization
*/
XBT_PRIVATE void unserialize_contiguous( const void *contiguous_vector, void *noncontiguous_vector, int count,
                         void *type, MPI_Op op);
XBT_PRIVATE void serialize_contiguous( const void *noncontiguous_vector, void *contiguous_vector, int count,void *type);
XBT_PRIVATE void free_contiguous(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_contiguous_t* smpi_datatype_contiguous_create( MPI_Aint lb, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype);
                                                  
XBT_PRIVATE void unserialize_vector( const void *contiguous_vector, void *noncontiguous_vector, int count,void *type,
                         MPI_Op op);
XBT_PRIVATE void serialize_vector( const void *noncontiguous_vector, void *contiguous_vector, int count, void *type);
XBT_PRIVATE void free_vector(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_vector_t* smpi_datatype_vector_create( int block_stride, int block_length, int block_count,
                                                  MPI_Datatype old_type, int size_oldtype);

XBT_PRIVATE void unserialize_hvector( const void *contiguous_vector, void *noncontiguous_vector, int count, void *type,
                         MPI_Op op);
XBT_PRIVATE void serialize_hvector( const void *noncontiguous_vector, void *contiguous_vector, int count, void *type);
XBT_PRIVATE void free_hvector(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_hvector_t* smpi_datatype_hvector_create( MPI_Aint block_stride, int block_length,
                                                  int block_count, MPI_Datatype old_type, int size_oldtype);

XBT_PRIVATE void unserialize_indexed( const void *contiguous_indexed, void *noncontiguous_indexed, int count,
                         void *type, MPI_Op op);
XBT_PRIVATE void serialize_indexed( const void *noncontiguous_vector, void *contiguous_vector, int count, void *type);
XBT_PRIVATE void free_indexed(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_indexed_t* smpi_datatype_indexed_create(int* block_lengths, int* block_indices,
                                                  int block_count, MPI_Datatype old_type, int size_oldtype);

XBT_PRIVATE void unserialize_hindexed( const void *contiguous_indexed, void *noncontiguous_indexed, int count,
                         void *type, MPI_Op op);
XBT_PRIVATE void serialize_hindexed( const void *noncontiguous_vector, void *contiguous_vector, int count, void *type);
XBT_PRIVATE void free_hindexed(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_hindexed_t* smpi_datatype_hindexed_create(int* block_lengths, MPI_Aint* block_indices,
                                                  int block_count, MPI_Datatype old_type, int size_oldtype);

XBT_PRIVATE void unserialize_struct( const void *contiguous_indexed, void *noncontiguous_indexed, int count, void *type,
                         MPI_Op op);
XBT_PRIVATE void serialize_struct( const void *noncontiguous_vector, void *contiguous_vector, int count, void *type);
XBT_PRIVATE void free_struct(MPI_Datatype* type);
XBT_PRIVATE s_smpi_mpi_struct_t* smpi_datatype_struct_create(int* block_lengths, MPI_Aint* block_indices,
                                                  int block_count, MPI_Datatype* old_types);

SG_END_DECL()
#endif
