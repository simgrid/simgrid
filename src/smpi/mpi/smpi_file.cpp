/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "private.hpp"

#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype.hpp"
#include "smpi_info.hpp"
#include "smpi_win.hpp"
#include "smpi_request.hpp"

#include "smpi_file.hpp"
#include "smpi_status.hpp"
#include "simgrid/s4u/Disk.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/plugins/file_system.h"

#define FP_SIZE sizeof(MPI_Offset)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_io, smpi, "Logging specific to SMPI (RMA operations)");

MPI_Errhandler SMPI_default_File_Errhandler = MPI_ERRORS_RETURN;

namespace simgrid{
namespace smpi{

  File::File(MPI_Comm comm, const char *filename, int amode, MPI_Info info): comm_(comm), flags_(amode), info_(info) {
    if (info_ != MPI_INFO_NULL)
      info_->ref();
    std::string fullname=filename;
    if (simgrid::s4u::Host::current()->get_disks().empty())
      xbt_die("SMPI/IO : Trying to open file on a diskless host ! Add one to your platform file");

    size_t found=fullname.find('/');
    //in case no fullpath is provided ... just pick the first mountpoint.
    if(found==std::string::npos){
      auto disk = simgrid::s4u::Host::current()->get_disks().front();
      std::string mount;
      if (disk->get_host() != simgrid::s4u::Host::current())
        mount = disk->extension<simgrid::s4u::FileSystemDiskExt>()->get_mount_point(disk->get_host());
      else
        mount = disk->extension<simgrid::s4u::FileSystemDiskExt>()->get_mount_point();
      XBT_DEBUG("No absolute path given for file opening, use '%s'", mount.c_str());
      mount.append("/");
      fullname.insert(0, mount);
    }

    file_= new simgrid::s4u::File(fullname, nullptr);
    list_=nullptr;
    if (comm_->rank() == 0) {
      int size= comm_->size() + FP_SIZE;
      list_ = new char[size];
      errhandler_= SMPI_default_File_Errhandler;
      errhandler_->ref();
      memset(list_, 0, size);
      shared_file_pointer_ = new MPI_Offset();
      shared_mutex_ = s4u::Mutex::create();
      *shared_file_pointer_ = 0;
      win_=new Win(list_, size, 1, MPI_INFO_NULL, comm_);
    }else{
      errhandler_ = MPI_ERRHANDLER_NULL;
      win_=new Win(list_, 0, 1, MPI_INFO_NULL, comm_);
    }
    simgrid::smpi::colls::bcast(&shared_file_pointer_, 1, MPI_AINT, 0, comm);
    simgrid::smpi::colls::bcast(&shared_mutex_, 1, MPI_AINT, 0, comm);
    if(comm_->rank() != 0)
      intrusive_ptr_add_ref(&*shared_mutex_);
  }

  File::~File(){
    if(comm_->rank() == 0){
      delete shared_file_pointer_;
      delete[] list_;
    }
    delete win_;
    delete file_;
    if (info_ != MPI_INFO_NULL)
      simgrid::smpi::Info::unref(info_);
    if (errhandler_ != MPI_ERRHANDLER_NULL)
      simgrid::smpi::Errhandler::unref(errhandler_);
  }

  int File::close(MPI_File *fh){
    XBT_DEBUG("Closing MPI_File %s", (*fh)->file_->get_path());
    (*fh)->sync();
    if((*fh)->flags() & MPI_MODE_DELETE_ON_CLOSE)
      (*fh)->file_->unlink();
    delete (*fh);
    return MPI_SUCCESS;
  }

  int File::del(const char* filename, const Info*)
  {
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
    shared_mutex_->lock();
    *offset=*shared_file_pointer_;
    shared_mutex_->unlock();
    return MPI_SUCCESS;
  }

  int File::seek(MPI_Offset offset, int whence){
    switch(whence){
      case(MPI_SEEK_SET):
        XBT_VERB("Seeking in MPI_File %s, setting offset %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_SET);
        break;
      case(MPI_SEEK_CUR):
        XBT_VERB("Seeking in MPI_File %s, current offset + %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_CUR);
        break;
      case(MPI_SEEK_END):
        XBT_VERB("Seeking in MPI_File %s, end offset + %lld", file_->get_path(), offset);
        file_->seek(offset,SEEK_END);
        break;
      default:
        return MPI_ERR_FILE;
    }
    return MPI_SUCCESS;
  }

  int File::seek_shared(MPI_Offset offset, int whence){
    shared_mutex_->lock();
    seek(offset,whence);
    *shared_file_pointer_=file_->tell();
    shared_mutex_->unlock();
    return MPI_SUCCESS;
  }

  int File::read(MPI_File fh, void* /*buf*/, int count, MPI_Datatype datatype, MPI_Status* status)
  {
    //get position first as we may be doing non contiguous reads and it will probably be updated badly
    MPI_Offset position = fh->file_->tell();
    MPI_Offset movesize = datatype->get_extent()*count;
    MPI_Offset readsize = datatype->size()*count;
    XBT_DEBUG("Position before read in MPI_File %s : %llu",fh->file_->get_path(),fh->file_->tell());
    MPI_Offset read = fh->file_->read(readsize);
    XBT_VERB("Read in MPI_File %s, %lld bytes read, readsize %lld bytes, movesize %lld", fh->file_->get_path(), read, readsize, movesize);
    if(readsize!=movesize){
      fh->file_->seek(position+movesize, SEEK_SET);
    }
    XBT_VERB("Position after read in MPI_File %s : %llu",fh->file_->get_path(), fh->file_->tell());
    if(status != MPI_STATUS_IGNORE)
      status->count=count*datatype->size();
    return MPI_SUCCESS;
  }

  /*Ordered and Shared Versions, with RMA-based locks : Based on the model described in :*/
  /* @InProceedings{10.1007/11557265_15,*/
  /* author="Latham, Robert and Ross, Robert and Thakur, Rajeev and Toonen, Brian",*/ 
  /* title="Implementing MPI-IO Shared File Pointers Without File System Support",*/
  /* booktitle="Recent Advances in Parallel Virtual Machine and Message Passing Interface",*/
  /* year="2005",*/
  /* publisher="Springer Berlin Heidelberg",*/
  /* address="Berlin, Heidelberg",*/
  /* pages="84--93"*/
  /* }*/
  int File::read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    fh->shared_mutex_->lock();
    fh->seek(*(fh->shared_file_pointer_),MPI_SEEK_SET);
    read(fh, buf, count, datatype, status);
    *(fh->shared_file_pointer_)=fh->file_->tell();
    fh->shared_mutex_->unlock();
    return MPI_SUCCESS;
  }

  int File::read_ordered(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //0 needs to get the shared pointer value
    MPI_Offset val;
    if(fh->comm_->rank()==0){
      val=*(fh->shared_file_pointer_);
    }else{
      val=count*datatype->size();
    }

    MPI_Offset result;
    simgrid::smpi::colls::scan(&val, &result, 1, MPI_OFFSET, MPI_SUM, fh->comm_);
    fh->seek(result, MPI_SEEK_SET);
    int ret = fh->op_all<simgrid::smpi::File::read>(buf, count, datatype, status);
    if(fh->comm_->rank()==fh->comm_->size()-1){
      fh->shared_mutex_->lock();
      *(fh->shared_file_pointer_)=fh->file_->tell();
      fh->shared_mutex_->unlock();
    }
    char c;
    simgrid::smpi::colls::bcast(&c, 1, MPI_BYTE, fh->comm_->size() - 1, fh->comm_);
    return ret;
  }

  int File::write(MPI_File fh, void* /*buf*/, int count, MPI_Datatype datatype, MPI_Status* status)
  {
    //get position first as we may be doing non contiguous reads and it will probably be updated badly
    MPI_Offset position = fh->file_->tell();
    MPI_Offset movesize = datatype->get_extent()*count;
    MPI_Offset writesize = datatype->size()*count;
    XBT_DEBUG("Position before write in MPI_File %s : %llu",fh->file_->get_path(),fh->file_->tell());
    MPI_Offset write = fh->file_->write(writesize, true);
    XBT_VERB("Write in MPI_File %s, %lld bytes written, readsize %lld bytes, movesize %lld", fh->file_->get_path(), write, writesize, movesize);
    if(writesize!=movesize){
      fh->file_->seek(position+movesize, SEEK_SET);
    }
    XBT_VERB("Position after write in MPI_File %s : %llu",fh->file_->get_path(), fh->file_->tell());
    if(status != MPI_STATUS_IGNORE)
      status->count=count*datatype->size();
    return MPI_SUCCESS;
  }

  int File::write_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    fh->shared_mutex_->lock();
    XBT_DEBUG("Write shared on %s - Shared ptr before : %lld",fh->file_->get_path(), *(fh->shared_file_pointer_));
    fh->seek(*(fh->shared_file_pointer_),MPI_SEEK_SET);
    write(fh, const_cast<void*>(buf), count, datatype, status);
    *(fh->shared_file_pointer_)=fh->file_->tell();
    XBT_DEBUG("Write shared on %s - Shared ptr after : %lld",fh->file_->get_path(), *(fh->shared_file_pointer_));
    fh->shared_mutex_->unlock();
    return MPI_SUCCESS;
  }

  int File::write_ordered(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //0 needs to get the shared pointer value
    MPI_Offset val;
    if(fh->comm_->rank()==0){
      val=*(fh->shared_file_pointer_);
    }else{
      val=count*datatype->size();
    }
    MPI_Offset result;
    simgrid::smpi::colls::scan(&val, &result, 1, MPI_OFFSET, MPI_SUM, fh->comm_);
    fh->seek(result, MPI_SEEK_SET);
    int ret = fh->op_all<simgrid::smpi::File::write>(const_cast<void*>(buf), count, datatype, status);
    if(fh->comm_->rank()==fh->comm_->size()-1){
      fh->shared_mutex_->lock();
      *(fh->shared_file_pointer_)=fh->file_->tell();
      fh->shared_mutex_->unlock();
    }
    char c;
    simgrid::smpi::colls::bcast(&c, 1, MPI_BYTE, fh->comm_->size() - 1, fh->comm_);
    return ret;
  }

  int File::set_view(MPI_Offset /*disp*/, MPI_Datatype etype, MPI_Datatype filetype, const char* datarep, const Info*)
  {
    etype_=etype;
    filetype_=filetype;
    datarep_=std::string(datarep);
    seek_shared(0,MPI_SEEK_SET);
    return MPI_SUCCESS;
  }

  int File::get_view(MPI_Offset* /*disp*/, MPI_Datatype* etype, MPI_Datatype* filetype, char* datarep)
  {
    *etype=etype_;
    *filetype=filetype_;
    snprintf(datarep, MPI_MAX_NAME_STRING+1, "%s", datarep_.c_str());
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
    return simgrid::smpi::colls::barrier(comm_);
  }

  MPI_Info File::info()
  {
    if (info_ == MPI_INFO_NULL)
      info_ = new Info();
    info_->ref();
    return info_;
  }

  void File::set_info(MPI_Info info)
  {
    if (info_ != MPI_INFO_NULL)
      simgrid::smpi::Info::unref(info_);
    info_ = info;
    if (info_ != MPI_INFO_NULL)
      info_->ref();
  }

  MPI_Comm File::comm(){
    return comm_;
  }

  MPI_Errhandler File::errhandler()
  {
    if (errhandler_ != MPI_ERRHANDLER_NULL)
      errhandler_->ref();
    return errhandler_;
  }

  void File::set_errhandler(MPI_Errhandler errhandler)
  {
    if (errhandler_ != MPI_ERRHANDLER_NULL)
      simgrid::smpi::Errhandler::unref(errhandler_);
    errhandler_ = errhandler;
    if (errhandler_ != MPI_ERRHANDLER_NULL)
      errhandler_->ref();
  }

  File* File::f2c(int id){
    return static_cast<File*>(F2C::f2c(id));
  }
}
}
