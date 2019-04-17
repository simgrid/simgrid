/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_file.hpp"
#include "smpi_datatype.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

int PMPI_File_open(MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *fh){
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (filename == nullptr) {
    return MPI_ERR_FILE;
  } else if (amode < 0) {
    return MPI_ERR_AMODE;
  } else {
    smpi_bench_end();
    *fh =  new simgrid::smpi::File(comm, filename, amode, info);
    smpi_bench_begin();
    if (((*fh)->size() == 0 && (not amode & MPI_MODE_CREATE)) ||
       ((*fh)->size() != 0 && (amode & MPI_MODE_EXCL))){
      delete fh;
      return MPI_ERR_AMODE;
    }
    if(amode & MPI_MODE_APPEND)
      (*fh)->seek(0,MPI_SEEK_END);
    return MPI_SUCCESS;
  }
}

int PMPI_File_close(MPI_File *fh){
  if (fh==nullptr){
    return MPI_ERR_ARG;
  } else {
    smpi_bench_end();
    int ret = simgrid::smpi::File::close(fh);
    *fh = MPI_FILE_NULL;
    smpi_bench_begin();
    return ret;
  }
}
#define CHECK_FILE(fh) if(fh==MPI_FILE_NULL) return MPI_ERR_FILE
#define CHECK_BUFFER(buf, count)  else if (buf==nullptr && count > 0) return MPI_ERR_BUFFER
#define CHECK_COUNT(count)  else if ( count < 0) return MPI_ERR_COUNT
#define CHECK_OFFSET(offset)  else if ( offset < 0) return MPI_ERR_DISP
#define PASS_ZEROCOUNT(count)  else if ( count == 0) return MPI_SUCCESS
#define CHECK_DATATYPE(datatype, count) else if ( datatype == MPI_DATATYPE_NULL && count > 0) return MPI_ERR_TYPE
#define CHECK_STATUS(status) else if (status == nullptr) return MPI_ERR_ARG
#define CHECK_FLAGS(fh) else if (fh->flags() & MPI_MODE_SEQUENTIAL) return MPI_ERR_AMODE

int PMPI_File_seek(MPI_File fh, MPI_Offset offset, int whence){
  CHECK_FILE(fh);
  else {
    smpi_bench_end();
    int ret = fh->seek(offset,whence);
    smpi_bench_begin();
    return ret;
  }
}

int PMPI_File_read(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE(fh);
  CHECK_BUFFER(buf, count);
  CHECK_COUNT(count);
  PASS_ZEROCOUNT(count);
  CHECK_DATATYPE(datatype, count);
  CHECK_STATUS(status);
  CHECK_FLAGS(fh);
  else {
    smpi_bench_end();
    int rank_traced = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - read", static_cast<double>(count*datatype->size())));
    int ret = simgrid::smpi::File::read(fh, buf, count, datatype, status);
    TRACE_smpi_comm_out(rank_traced);
    smpi_bench_begin();
    return ret;
  }
}

int PMPI_File_write(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE(fh);
  CHECK_BUFFER(buf, count);
  CHECK_COUNT(count);
  PASS_ZEROCOUNT(count);
  CHECK_DATATYPE(datatype, count);
  CHECK_STATUS(status);
  CHECK_FLAGS(fh);
  else {
    smpi_bench_end();
    int rank_traced = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - write", static_cast<double>(count*datatype->size())));
    int ret = simgrid::smpi::File::write(fh, buf, count, datatype, status);
    TRACE_smpi_comm_out(rank_traced);
    smpi_bench_begin();
    return ret;
  }
}

int PMPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE(fh);
  CHECK_BUFFER(buf, count);
  CHECK_OFFSET(offset);
  CHECK_COUNT(count);
  PASS_ZEROCOUNT(count);
  CHECK_DATATYPE(datatype, count);
  CHECK_STATUS(status);
  CHECK_FLAGS(fh);
  else {
    smpi_bench_end();
    int rank_traced = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - read", static_cast<double>(count*datatype->size())));
    int ret = fh->seek(offset,SEEK_SET);
    if(ret!=MPI_SUCCESS)
      return ret;
    ret = simgrid::smpi::File::read(fh, buf, count, datatype, status);
    TRACE_smpi_comm_out(rank_traced);
    smpi_bench_begin();
    return ret;
  }
}


int PMPI_File_write_at(MPI_File fh, MPI_Offset offset, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE(fh);
  CHECK_BUFFER(buf, count);
  CHECK_OFFSET(offset);
  CHECK_COUNT(count);
  PASS_ZEROCOUNT(count);
  CHECK_DATATYPE(datatype, count);
  CHECK_STATUS(status);
  CHECK_FLAGS(fh);
  else {
    smpi_bench_end();
    int rank_traced = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - write", static_cast<double>(count*datatype->size())));
    int ret = fh->seek(offset,SEEK_SET);
    if(ret!=MPI_SUCCESS)
      return ret;
    ret = simgrid::smpi::File::write(fh, buf, count, datatype, status);
    TRACE_smpi_comm_out(rank_traced);
    smpi_bench_begin();
    return ret;
  }
}

int PMPI_File_delete(char *filename, MPI_Info info){
  if (filename == nullptr) {
    return MPI_ERR_FILE;
  } else {
    smpi_bench_end();
    int ret = simgrid::smpi::File::del(filename, info);
    smpi_bench_begin();
    return ret;
  }
}

