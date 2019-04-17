/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "private.hpp"

#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype.hpp"
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
    XBT_DEBUG("Closing MPI_File %s", (*fh)->file_->get_path());
    (*fh)->sync();
    if((*fh)->flags() & MPI_MODE_DELETE_ON_CLOSE)
      (*fh)->file_->unlink();
    delete (*fh);
    return MPI_SUCCESS;
  }

  int File::del(char *filename, MPI_Info info){
    //get the file with MPI_MODE_DELETE_ON_CLOSE and then close it
    File* f = new File(MPI_COMM_SELF,filename,MPI_MODE_DELETE_ON_CLOSE|MPI_MODE_RDWR, nullptr);
    close(&f);
    return MPI_SUCCESS;
  }

  int File::seek(MPI_Offset offset, int whence){
    switch(whence){
      case(MPI_SEEK_SET):
        XBT_DEBUG("Seeking in MPI_File %s, setting offset %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_SET);
        break;
      case(MPI_SEEK_CUR):
        XBT_DEBUG("Seeking in MPI_File %s, current offset + %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_CUR);
        break;
      case(MPI_SEEK_END):
        XBT_DEBUG("Seeking in MPI_File %s, end offset + %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_END);
        break;
      default:
        return MPI_ERR_FILE;
    }
    return MPI_SUCCESS;
  }
  
  int File::read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //get position first as we may be doing non contiguous reads and it will probably be updated badly
    MPI_Offset position = fh->file_->tell();
    MPI_Offset movesize = datatype->get_extent()*count;
    MPI_Offset readsize = datatype->size()*count;
    XBT_DEBUG("Position before read in MPI_File %s : %llu",fh->file_->get_path(),fh->file_->tell());
    MPI_Offset read = fh->file_->read(readsize);
    XBT_DEBUG("Read in MPI_File %s, %lld bytes read, readsize %lld bytes, movesize %lld", fh->file_->get_path(), read, readsize, movesize);
    if(readsize!=movesize){
      fh->file_->seek(position+movesize, SEEK_SET);
    }
    XBT_DEBUG("Position after read in MPI_File %s : %llu",fh->file_->get_path(), fh->file_->tell());
    return MPI_SUCCESS;
  }


  int File::write(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //get position first as we may be doing non contiguous reads and it will probably be updated badly
    MPI_Offset position = fh->file_->tell();
    MPI_Offset movesize = datatype->get_extent()*count;
    MPI_Offset writesize = datatype->size()*count;
    XBT_DEBUG("Position before write in MPI_File %s : %llu",fh->file_->get_path(),fh->file_->tell());
    MPI_Offset write = fh->file_->write(writesize);
    XBT_DEBUG("Write in MPI_File %s, %lld bytes read, readsize %lld bytes, movesize %lld", fh->file_->get_path(), write, writesize, movesize);
    if(writesize!=movesize){
      fh->file_->seek(position+movesize, SEEK_SET);
    }
    XBT_DEBUG("Position after write in MPI_File %s : %llu",fh->file_->get_path(), fh->file_->tell());
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
