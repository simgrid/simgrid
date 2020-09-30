/* Copyright (c) 2010-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_ERRHANDLER_HPP_INCLUDED
#define SMPI_ERRHANDLER_HPP_INCLUDED
#include "smpi_f2c.hpp"
#include <smpi/smpi.h>


namespace simgrid{
namespace smpi{

class Errhandler: public F2C {
  private:
    int refcount_ = 1;
    MPI_Comm_errhandler_fn* comm_func_=nullptr;
    MPI_File_errhandler_fn* file_func_=nullptr;
    MPI_Win_errhandler_fn* win_func_=nullptr;
  public:
  Errhandler() = default;
  explicit Errhandler(MPI_Comm_errhandler_fn *function):comm_func_(function){};
  explicit Errhandler(MPI_File_errhandler_fn *function):file_func_(function){};
  explicit Errhandler(MPI_Win_errhandler_fn *function):win_func_(function){};
  void ref();
  void call(MPI_Comm comm, int errorcode) const;
  void call(MPI_Win win, int errorcode) const;
  void call(MPI_File file, int errorcode) const;
  static void unref(Errhandler* errhandler);
  static Errhandler* f2c(int id);
};
}
}

#endif
