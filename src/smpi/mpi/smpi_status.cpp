/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_status.hpp"
#include "private.hpp"
#include "smpi_datatype.hpp"
#include "src/simix/smx_private.hpp"

namespace simgrid{
namespace smpi{

void Status::empty(MPI_Status * status)
{
  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG = MPI_ANY_TAG;
    status->MPI_ERROR = MPI_SUCCESS;
    status->count=0;
    status->cancelled=0;
  }
}

int Status::cancelled(const MPI_Status * status)
{
  return status->cancelled!=0;
}

void Status::set_cancelled(MPI_Status * status, int flag)
{
  status->cancelled=flag;
}

void Status::set_elements(MPI_Status* status, const Datatype*, int count)
{
  status->count=count;
}

int Status::get_count(const MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / datatype->size();
}

}
}
