/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_errhandler.hpp"
#include "private.hpp"

#include <cstdio>

simgrid::smpi::Errhandler smpi_MPI_ERRORS_RETURN;
simgrid::smpi::Errhandler smpi_MPI_ERRORS_ARE_FATAL;

namespace simgrid{
namespace smpi{

MPI_Errhandler Errhandler::f2c(int id) {
  if (F2C::lookup() != nullptr && id >= 0) {
    return static_cast<MPI_Errhandler>(F2C::lookup()->at(id));
  } else {
    return MPI_ERRHANDLER_NULL;
  }
}

void Errhandler::call(MPI_Comm comm, int errorcode) const
{
  comm_func_(&comm, &errorcode);
}

void Errhandler::call(MPI_Win win, int errorcode) const
{
  win_func_(&win, &errorcode);
}

void Errhandler::call(MPI_File file, int errorcode) const
{
  file_func_(&file, &errorcode);
}

void Errhandler::ref()
{
  refcount_++;
}

void Errhandler::unref(Errhandler* errhandler){
  if(errhandler == MPI_ERRORS_ARE_FATAL || errhandler == MPI_ERRORS_RETURN)
    return;
  errhandler->refcount_--;
  if(errhandler->refcount_==0){
    F2C::free_f(errhandler->f2c_id());
    delete errhandler;
  }
}

}

}
