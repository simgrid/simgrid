/* Copyright (c) 2009-2020. The SimGrid Team.
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
  Type_Contiguous(const Type_Contiguous&) = delete;
  Type_Contiguous& operator=(const Type_Contiguous&) = delete;
  ~Type_Contiguous() override;
  int clone(MPI_Datatype* type) override;
  void serialize(const void* noncontiguous, void* contiguous, int count) override;
  void unserialize(const void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op) override;
};

class Type_Hvector: public Datatype{
public:
  int block_count_;
  int block_length_;
  MPI_Aint block_stride_;
  MPI_Datatype old_type_;

  Type_Hvector(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, int block_length, MPI_Aint block_stride,
               MPI_Datatype old_type);
  Type_Hvector(const Type_Hvector&) = delete;
  Type_Hvector& operator=(const Type_Hvector&) = delete;
  ~Type_Hvector() override;
  int clone(MPI_Datatype* type) override;
  void serialize(const void* noncontiguous, void* contiguous, int count) override;
  void unserialize(const void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op) override;
};

class Type_Vector : public Type_Hvector {
public:
  Type_Vector(int size, MPI_Aint lb, MPI_Aint ub, int flags, int count, int blocklen, int stride,
              MPI_Datatype old_type);
  int clone(MPI_Datatype* type) override;
};

class Type_Hindexed: public Datatype{
public:
  int block_count_;
  int* block_lengths_;
  MPI_Aint* block_indices_;
  MPI_Datatype old_type_;

  Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, const int* block_lengths,
                const MPI_Aint* block_indices, MPI_Datatype old_type);
  Type_Hindexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, const int* block_lengths, const int* block_indices,
                MPI_Datatype old_type, MPI_Aint factor);
  Type_Hindexed(const Type_Hindexed&) = delete;
  Type_Hindexed& operator=(const Type_Hindexed&) = delete;
  int clone(MPI_Datatype* type) override;
  ~Type_Hindexed() override;
  void serialize(const void* noncontiguous, void* contiguous, int count) override;
  void unserialize(const void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op) override;
};

class Type_Indexed : public Type_Hindexed {
public:
  Type_Indexed(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, const int* block_lengths, const int* block_indices,
               MPI_Datatype old_type);
  int clone(MPI_Datatype* type) override;
};

class Type_Struct: public Datatype{
  int block_count_;
  int* block_lengths_;
  MPI_Aint* block_indices_;
  MPI_Datatype* old_types_;

public:
  Type_Struct(int size, MPI_Aint lb, MPI_Aint ub, int flags, int block_count, const int* block_lengths,
              const MPI_Aint* block_indices, const MPI_Datatype* old_types);
  Type_Struct(const Type_Struct&) = delete;
  Type_Struct& operator=(const Type_Struct&) = delete;
  int clone(MPI_Datatype* type) override;
  ~Type_Struct() override;
  void serialize(const void* noncontiguous, void* contiguous, int count) override;
  void unserialize(const void* contiguous_vector, void* noncontiguous_vector, int count, MPI_Op op) override;
};

} // namespace smpi
} // namespace simgrid

#endif
