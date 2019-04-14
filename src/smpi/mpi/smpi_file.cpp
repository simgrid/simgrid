/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "private.hpp"

#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_info.hpp"
#include "smpi_file.hpp"
#include "simgrid/plugins/file_system.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_io, smpi, "Logging specific to SMPI (RMA operations)");


namespace simgrid{
namespace smpi{
  File::File(MPI_Comm comm, char *filename, int amode, MPI_Info info): comm_(comm), flags_(amode), info_(info){
    file_= new simgrid::s4u::File(filename, nullptr);
  }
  
  File::~File(){
    delete file_;
  }
  
  int File::close(MPI_File *fh){
    (*fh)->sync();
    if((*fh)->flags() & MPI_MODE_DELETE_ON_CLOSE)
      (*fh)->file_->unlink();
    delete fh;
    return MPI_SUCCESS;
  }
  
  int File::del(char *filename, MPI_Info info){
    File* f = new File(MPI_COMM_SELF,filename,MPI_MODE_DELETE_ON_CLOSE|MPI_MODE_RDWR, nullptr);
    close(&f);
    return MPI_SUCCESS;
  }
  
  int File::size(){
    return file_->size();
  }
  
  int File::flags(){
    return flags_;
  }
  int File::sync(){
    //no idea
    return simgrid::smpi::Colls::barrier(comm_);
  }
}
}