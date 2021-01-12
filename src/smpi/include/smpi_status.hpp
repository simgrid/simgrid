/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_STATUS_HPP
#define SMPI_STATUS_HPP

#include "smpi/smpi.h"

namespace simgrid{
namespace smpi{

//Status has to stay in C, as its fields are public.
//So status class only defines static methods to handle the C struct.
class Status{
public:
static void empty(MPI_Status * status);
static int cancelled (const MPI_Status * status);
static void set_cancelled (MPI_Status * status, int flag);
static void set_elements(MPI_Status* status, const Datatype*, int count);
static int get_count(const MPI_Status* status, const Datatype* datatype);
};


}
}
#endif
