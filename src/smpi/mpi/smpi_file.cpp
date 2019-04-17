/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "private.hpp"

#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype.hpp"
#include "smpi_info.hpp"
#include "smpi_win.hpp"
#include "smpi_file.hpp"
#include "smpi_status.hpp"
#include "simgrid/plugins/file_system.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_io, smpi, "Logging specific to SMPI (RMA operations)");
#define FP_SIZE sizeof(MPI_Offset)


namespace simgrid{
namespace smpi{

  File::File(MPI_Comm comm, char *filename, int amode, MPI_Info info): comm_(comm), flags_(amode), info_(info), shared_file_pointer_(0) {
    file_= new simgrid::s4u::File(filename, nullptr);
    list_=nullptr;
    if (comm_->rank() == 0) {
      int size= comm_->size() + FP_SIZE;
      list_ = new char[size];
      memset(list_, 0, size);
      win_=new Win(list_, size, 1, MPI_INFO_NULL, comm_);
    }else{
      win_=new Win(list_, 0, 1, MPI_INFO_NULL, comm_);
    }
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

  int File::get_position(MPI_Offset* offset){
    *offset=file_->tell();
    return MPI_SUCCESS;
  }

  int File::get_position_shared(MPI_Offset* offset){
    lock();
    *offset=shared_file_pointer_;
    unlock();
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

  int File::seek_shared(MPI_Offset offset, int whence){
    lock();
    seek(offset,whence);
    shared_file_pointer_=file_->tell();
    unlock();
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
    status->count=count*datatype->size();
    return MPI_SUCCESS;
  }

  int File::read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    fh->lock();
    fh->seek(fh->shared_file_pointer_,MPI_SEEK_SET);
    read(fh, buf, count, datatype, status);
    fh->shared_file_pointer_=fh->file_->tell();
    fh->unlock();
    return MPI_SUCCESS;
  }

  int File::read_ordered(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //0 needs to get the shared pointer value
    if(fh->comm_->rank()==0){
      fh->lock();
      fh->unlock();
    }else{
      fh->shared_file_pointer_=count*datatype->size();
    }
    MPI_Offset result;
    simgrid::smpi::Colls::scan(&(fh->shared_file_pointer_), &result, 1, MPI_OFFSET, MPI_SUM, fh->comm_);
    fh->seek(result, MPI_SEEK_SET);
    int ret = fh->op_all<simgrid::smpi::File::read>(buf, count, datatype, status);
    if(fh->comm_->rank()==fh->comm_->size()-1){
      fh->lock();
      fh->unlock();
    }
    char c;
    simgrid::smpi::Colls::bcast(&c, 1, MPI_BYTE, fh->comm_->size()-1, fh->comm_);
    return ret;
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
    status->count=count*datatype->size();
    return MPI_SUCCESS;
  }

  int File::write_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    fh->lock();
    fh->seek(fh->shared_file_pointer_,MPI_SEEK_SET);
    write(fh, buf, count, datatype, status);
    fh->shared_file_pointer_=fh->file_->tell();
    fh->unlock();
    return MPI_SUCCESS;
  }

  int File::write_ordered(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //0 needs to get the shared pointer value
    if(fh->comm_->rank()==0){
      fh->lock();
      fh->unlock();
    }else{
      fh->shared_file_pointer_=count*datatype->size();
    }
    MPI_Offset result;
    simgrid::smpi::Colls::scan(&(fh->shared_file_pointer_), &result, 1, MPI_OFFSET, MPI_SUM, fh->comm_);
    fh->seek(result, MPI_SEEK_SET);
    int ret = fh->op_all<simgrid::smpi::File::write>(buf, count, datatype, status);
    if(fh->comm_->rank()==fh->comm_->size()-1){
      fh->lock();
      fh->unlock();
    }
    char c;
    simgrid::smpi::Colls::bcast(&c, 1, MPI_BYTE, fh->comm_->size()-1, fh->comm_);
    return ret;
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

  int File::lock()
{
  int rank = comm_->rank();
  int size = comm_->size();
  char waitlist[size];
  char lock = 1;
  int tag=444;
  int i;
  win_->lock(MPI_LOCK_EXCLUSIVE, 0, 0);
  win_->put(&lock, 1, MPI_CHAR, 0, FP_SIZE+rank, 1, MPI_CHAR);
  win_->get(waitlist, size, MPI_CHAR, 0, FP_SIZE, size, MPI_CHAR);
  win_->get(&shared_file_pointer_ , 1 , MPI_OFFSET , 0 , 0, 1, MPI_OFFSET);
  win_->unlock(0);
  for (i = 0; i < size; i++) {
    if (waitlist[i] == 1 && i != rank) {
      // wait for the lock
      MPI_Recv(&lock, 1, MPI_CHAR, MPI_ANY_SOURCE, tag, comm_, MPI_STATUS_IGNORE);
      break;
    }
  }
  return 0;
}

int File::unlock()
{
  int rank = comm_->rank();
  int size = comm_->size();
  char waitlist[size];
  char lock = 0;
  int tag=444;
  int i, next;
  win_->lock(MPI_LOCK_EXCLUSIVE, 0, 0);
  win_->put(&lock, 1, MPI_CHAR, 0, FP_SIZE+rank, 1, MPI_CHAR);
  win_->get(waitlist, size, MPI_CHAR, 0, FP_SIZE, size, MPI_CHAR);
  shared_file_pointer_=file_->tell();
  win_->put(&shared_file_pointer_, 1 , MPI_OFFSET , 0 , 0, 1, MPI_OFFSET);

  win_->unlock(0);
  next = (rank + 1 + size) % size;
  for (i = 0; i < size; i++, next = (next + 1) % size) {
    if (waitlist[next] == 1) {
      MPI_Send(&lock, 1, MPI_CHAR, next, tag, comm_);
      break;
    }
  }
  return 0;
}

MPI_Info File::info(){
  if(info_== MPI_INFO_NULL)
    info_ = new Info();
  info_->ref();
  return info_;
}

void File::set_info(MPI_Info info){
  if(info_!= MPI_INFO_NULL)
    info->ref();
  info_=info;
}

}
}
