/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_OP_HPP
#define SMPI_OP_HPP

#include "smpi_info.hpp"

namespace simgrid::smpi {

class Op : public F2C{
  MPI_User_function* func_;
  bool is_commutative_;
  bool is_fortran_op_ = false;
  int refcount_ = 1;
  bool is_predefined_;
  int types_; //bitmask of the allowed datatypes flags
  std::string name_;

public:
  Op(MPI_User_function* function, bool commutative, bool predefined = false, int types = 0, std::string name = "MPI_Op")
      : func_(function), is_commutative_(commutative), is_predefined_(predefined), types_(types), name_(std::move(name))
  {
    if (not predefined)
      this->add_f();
  }
  bool is_commutative() const { return is_commutative_; }
  bool is_predefined() const { return is_predefined_; }
  bool is_fortran_op() const { return is_fortran_op_; }
  int allowed_types() const { return types_; }
  std::string name() const override {return name_;}
  // tell that we were created from fortran, so we need to translate the type to fortran when called
  void set_fortran_op() { is_fortran_op_ = true; }
  void apply(const void* invec, void* inoutvec, const int* len, MPI_Datatype datatype) const;
  static Op* f2c(int id);
  void ref();
  static void unref(MPI_Op* op);
};

} // namespace simgrid::smpi

#endif
