/* Handle Fortran - C conversion for MPI Types*/

/* Copyright (c) 2010-2020. The SimGrid Team.
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
    static std::unordered_map<int, F2C*>* f2c_lookup_;
    static int f2c_id_;
    int my_f2c_id_ = -1;

  protected:
    static std::unordered_map<int, F2C*>* f2c_lookup();
    static void set_f2c_lookup(std::unordered_map<int, F2C*>* map);
    static int f2c_id();
    static void f2c_id_increment();

  public:
    static void delete_lookup();
    static std::unordered_map<int, F2C*>* lookup();
    F2C();
    virtual ~F2C() = default;

    //Override these to handle specific values.
    virtual int add_f();
    static void free_f(int id);
    virtual int c2f();
    static void print_f2c_lookup();
    // This method should be overridden in all subclasses to avoid casting the result when calling it.
    // For the default one, the MPI_*_NULL returned is assumed to be NULL.
    static F2C* f2c(int id);
};

}
}

#endif
