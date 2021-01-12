/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_OP_HPP
#define SMPI_OP_HPP

#include "smpi_info.hpp"

namespace simgrid{
namespace smpi{

class Op : public F2C{
  MPI_User_function* func_;
  bool is_commutative_;
  bool is_fortran_op_ = false;
  int refcount_ = 1;
  bool predefined_;

public:
  Op(MPI_User_function* function, bool commutative, bool predefined=false) : func_(function), is_commutative_(commutative), predefined_(predefined) {}
  bool is_commutative() const { return is_commutative_; }
  bool is_fortran_op() const { return is_fortran_op_; }
  // tell that we were created from fortran, so we need to translate the type to fortran when called
  void set_fortran_op() { is_fortran_op_ = true; }
  void apply(const void* invec, void* inoutvec, const int* len, MPI_Datatype datatype) const;
  static Op* f2c(int id);
  void ref();
  static void unref(MPI_Op* op);
};

}
}

#endif
