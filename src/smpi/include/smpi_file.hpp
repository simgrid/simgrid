/* Copyright (c) 2010-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_FILE_HPP_INCLUDED
#define SMPI_FILE_HPP_INCLUDED
#include "simgrid/plugins/file_system.h"


namespace simgrid{
namespace smpi{
class File{
  MPI_Comm comm_;
  int flags_;
  simgrid::s4u::File* file_;
  MPI_Info info_;
  public:
  File(MPI_Comm comm, char *filename, int amode, MPI_Info info);
  ~File();
  int size();
  int flags();
  int sync();
  int seek(MPI_Offset offset, int whence);
  int read(void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int close(MPI_File *fh);
  static int del(char *filename, MPI_Info info);
};
}
}
#endif
