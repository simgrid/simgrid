/* Handle Fortran - C conversion for MPI Types*/

/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_F2C_HPP_INCLUDED
#define SMPI_F2C_HPP_INCLUDED

#include <unordered_map>

namespace simgrid{
namespace smpi{

class F2C {
private:
  // We use a single lookup table for every type.
  // Beware of collisions if id in mpif.h is not unique
  static std::unordered_map<unsigned int, F2C*>* f2c_lookup_;
  static int f2c_id_;
  static std::unordered_map<unsigned int, F2C*>::size_type num_default_handles_;
  int my_f2c_id_ = -1;

protected:
  static void set_f2c_lookup(std::unordered_map<unsigned int, F2C*>* map) { f2c_lookup_ = map; }
  static int f2c_id() { return f2c_id_; }
  static void f2c_id_increment() { f2c_id_++; }

public:
  static void delete_lookup() { delete f2c_lookup_; f2c_lookup_ = nullptr ;}
  static std::unordered_map<unsigned int, F2C*>* lookup() { return f2c_lookup_; }
  F2C();
  virtual ~F2C() = default;

  int add_f();
  static void free_f(int id) { f2c_lookup_->erase(id); }
  int c2f();
  static void print_f2c_lookup();
  // This method should be overridden in all subclasses to avoid casting the result when calling it.
  // For the default one, the MPI_*_NULL returned is assumed to be NULL.
  static F2C* f2c(int id);
  static void finish_initialization() { num_default_handles_ = f2c_lookup_->size(); }
  static std::unordered_map<unsigned int, F2C*>::size_type get_num_default_handles() { return num_default_handles_;}
};

}
}

#endif
