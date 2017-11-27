/* Copyright (c) 2009-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_DATATYPE_DERIVED_HPP
#define SMPI_DATATYPE_DERIVED_HPP

#include "smpi_datatype.hpp"

namespace simgrid{
namespace smpi{

class Type_Contiguous: public Datatype {
  int block_count_;
  MPI_Datatype old_type_;

public:
  Type_Contiguous(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, MPI_Datatype old_type);
  ~Type_Contiguous();
  void serialize(void* noncontiguous, void* contiguous, int count);
  void unserialize(void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op);
};

class Type_Hvector: public Datatype{
  int block_count_;
  int block_length_;
  MPI_Aint block_stride_;
  MPI_Datatype old_type_;

public:
  Type_Hvector(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int block_length, MPI_Aint block_stride,
               MPI_Datatype old_type);
  ~Type_Hvector();
  void serialize(void* noncontiguous, void* contiguous, int count);
  void unserialize(void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op);
};

class Type_Vector : public Type_Hvector {
public:
  Type_Vector(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, int blocklen, int stride,
              MPI_Datatype old_type);
};

class Type_Hindexed: public Datatype{
  int block_count_;
  int* block_lengths_;
  MPI_Aint* block_indices_;
  MPI_Datatype old_type_;

public:
  Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int* block_lengths,
                MPI_Aint* block_indices, MPI_Datatype old_type);
  Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int* block_lengths, int* block_indices,
                MPI_Datatype old_type, MPI_Aint factor);
  ~Type_Hindexed();
  void serialize(void* noncontiguous, void* contiguous, int count);
  void unserialize(void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op);
};

class Type_Indexed : public Type_Hindexed {
public:
  Type_Indexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int* block_lengths, int* block_indices,
               MPI_Datatype old_type);
};

class Type_Struct: public Datatype{
  int block_count_;
  int* block_lengths_;
  MPI_Aint* block_indices_;
  MPI_Datatype* old_types_;

public:
  Type_Struct(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int* block_lengths,
              MPI_Aint* block_indices, MPI_Datatype* old_types);
  ~Type_Struct();
  void serialize(void* noncontiguous, void* contiguous, int count);
  void unserialize(void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op);
};


}
}

#endif
