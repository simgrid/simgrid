/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"

#include "smpi_file.hpp"
#include "smpi_datatype.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);
#define CHECK_RDONLY(fh)                                                                                               \
  if ((fh)->flags() & MPI_MODE_RDONLY)                                                                                 \
    return MPI_ERR_AMODE;
#define CHECK_WRONLY(fh)                                                                                               \
  if ((fh)->flags() & MPI_MODE_WRONLY)                                                                                 \
    return MPI_ERR_AMODE;
#define PASS_ZEROCOUNT(count)                                                                                          \
  if ((count) == 0) {                                                                                                  \
    status->count = 0;                                                                                                 \
    return MPI_SUCCESS;                                                                                                \
  }

#define CHECK_FILE_INPUTS                                                                                              \
  CHECK_FILE(1, fh)                                                                                                    \
  CHECK_BUFFER(2, buf, count)                                                                                          \
  CHECK_COUNT(3, count)                                                                                                \
  CHECK_TYPE(4, datatype)                                                                                              \

#define CHECK_FILE_INPUT_OFFSET                                                                                        \
  CHECK_FILE(1, fh)                                                                                                    \
  CHECK_BUFFER(2, buf, count)                                                                                          \
  CHECK_OFFSET(3, offset)                                                                                                 \
  CHECK_COUNT(4, count)                                                                                                \
  CHECK_TYPE(5, datatype)                                                                                              \

extern MPI_Errhandler SMPI_default_File_Errhandler;

int PMPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh){
  CHECK_COMM(1)
  CHECK_NULL(2, MPI_ERR_FILE, filename)
  if (amode < 0)
    return MPI_ERR_AMODE;
  smpi_bench_end();
  *fh =  new simgrid::smpi::File(comm, filename, amode, info);
  smpi_bench_begin();
  if ((*fh)->size() != 0 && (amode & MPI_MODE_EXCL)){
    delete fh;
    return MPI_ERR_AMODE;
  }
  if(amode & MPI_MODE_APPEND)
    (*fh)->seek(0,MPI_SEEK_END);
  return MPI_SUCCESS;
}

int PMPI_File_close(MPI_File *fh){
  CHECK_NULL(2, MPI_ERR_ARG, fh)
  smpi_bench_end();
  int ret = simgrid::smpi::File::close(fh);
  *fh = MPI_FILE_NULL;
  smpi_bench_begin();
  return ret;
}

int PMPI_File_seek(MPI_File fh, MPI_Offset offset, int whence){
  CHECK_FILE(1, fh)
  smpi_bench_end();
  int ret = fh->seek(offset,whence);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence){
  CHECK_FILE(1, fh)
  smpi_bench_end();
  int ret = fh->seek_shared(offset,whence);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_get_position(MPI_File fh, MPI_Offset* offset){
  CHECK_FILE(1, fh)
  CHECK_NULL(2, MPI_ERR_DISP, offset)
  smpi_bench_end();
  int ret = fh->get_position(offset);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_get_position_shared(MPI_File fh, MPI_Offset* offset){
  CHECK_FILE(1, fh)
  CHECK_NULL(2, MPI_ERR_DISP, offset)
  smpi_bench_end();
  int ret = fh->get_position_shared(offset);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_WRONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - read", count * datatype->size()));
  int ret = simgrid::smpi::File::read(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read_shared(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_WRONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - read_shared", count * datatype->size()));
  int ret = simgrid::smpi::File::read_shared(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_RDONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - write", count * datatype->size()));
  int ret = simgrid::smpi::File::write(fh, const_cast<void*>(buf), count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write_shared(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_RDONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - write_shared", count * datatype->size()));
  int ret = simgrid::smpi::File::write_shared(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read_all(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_WRONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - read_all", count * datatype->size()));
  int ret = fh->op_all<simgrid::smpi::File::read>(buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read_ordered(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_WRONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - read_ordered", count * datatype->size()));
  int ret = simgrid::smpi::File::read_ordered(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write_all(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_RDONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - write_all", count * datatype->size()));
  int ret = fh->op_all<simgrid::smpi::File::write>(const_cast<void*>(buf), count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write_ordered(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_RDONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - write_ordered", count * datatype->size()));
  int ret = simgrid::smpi::File::write_ordered(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUTS
  CHECK_WRONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - read", count * datatype->size()));
  int ret = fh->seek(offset,MPI_SEEK_SET);
  if(ret!=MPI_SUCCESS)
    return ret;
  ret = simgrid::smpi::File::read(fh, buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUT_OFFSET
  CHECK_WRONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - read_at_all", count * datatype->size()));
  int ret = fh->seek(offset,MPI_SEEK_SET);
  if(ret!=MPI_SUCCESS)
    return ret;
  ret = fh->op_all<simgrid::smpi::File::read>(buf, count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUT_OFFSET
  CHECK_RDONLY(fh)
  PASS_ZEROCOUNT(count)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("IO - write", count * datatype->size()));
  int ret = fh->seek(offset,MPI_SEEK_SET);
  if(ret!=MPI_SUCCESS)
    return ret;
  ret = simgrid::smpi::File::write(fh, const_cast<void*>(buf), count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status){
  CHECK_FILE_INPUT_OFFSET
  CHECK_RDONLY(fh)
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__,
                     new simgrid::instr::CpuTIData("IO - write_at_all", count * datatype->size()));
  int ret = fh->seek(offset,MPI_SEEK_SET);
  if(ret!=MPI_SUCCESS)
    return ret;
  ret = fh->op_all<simgrid::smpi::File::write>(const_cast<void*>(buf), count, datatype, status);
  TRACE_smpi_comm_out(rank_traced);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_delete(const char *filename, MPI_Info info){
  if (filename == nullptr)
    return MPI_ERR_FILE;
  smpi_bench_end();
  int ret = simgrid::smpi::File::del(filename, info);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info){
  CHECK_FILE(1, fh)
  if(not ((fh->flags() & MPI_MODE_SEQUENTIAL) && (disp == MPI_DISPLACEMENT_CURRENT)))
    CHECK_OFFSET(2, disp)
  CHECK_TYPE(3, etype)
  CHECK_TYPE(4, filetype)
  smpi_bench_end();
  int ret = fh->set_view(disp, etype, filetype, datarep, info);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_get_view(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep){
  CHECK_FILE(1, fh)
  CHECK_NULL(2, MPI_ERR_ARG, disp)
  CHECK_NULL(3, MPI_ERR_ARG, etype)
  CHECK_NULL(4, MPI_ERR_ARG, filetype)
  smpi_bench_end();
  int ret = fh->get_view(disp, etype, filetype, datarep);
  smpi_bench_begin();
  return ret;
}

int PMPI_File_get_info(MPI_File  fh, MPI_Info* info)
{
  CHECK_FILE(1, fh)
  *info = new simgrid::smpi::Info(fh->info());
  return MPI_SUCCESS;
}

int PMPI_File_set_info(MPI_File  fh, MPI_Info info)
{
  CHECK_FILE(1, fh)
  fh->set_info(info);
  return MPI_SUCCESS;
}

int PMPI_File_get_size(MPI_File  fh, MPI_Offset* size)
{
  CHECK_FILE(1, fh)
  *size = fh->size();
  return MPI_SUCCESS;
}

int PMPI_File_set_size(MPI_File  fh, MPI_Offset size)
{
  CHECK_FILE(1, fh)
  fh->set_size(size);
  return MPI_SUCCESS;
}

int PMPI_File_get_amode(MPI_File  fh, int* amode)
{
  CHECK_FILE(1, fh)
  *amode = fh->flags();
  return MPI_SUCCESS;
}

int PMPI_File_get_group(MPI_File  fh, MPI_Group* group)
{
  CHECK_FILE(1, fh)
  *group = fh->comm()->group();
  return MPI_SUCCESS;
}

int PMPI_File_sync(MPI_File  fh)
{
  CHECK_FILE(1, fh)
  fh->sync();
  return MPI_SUCCESS;
}

int PMPI_File_create_errhandler(MPI_File_errhandler_function* function, MPI_Errhandler* errhandler){
  *errhandler=new simgrid::smpi::Errhandler(function);
  return MPI_SUCCESS;
}

int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler* errhandler){
  if (errhandler==nullptr){
    return MPI_ERR_ARG;
  } else if (file == MPI_FILE_NULL) {
    *errhandler = SMPI_default_File_Errhandler;
    return MPI_SUCCESS;
  }
  *errhandler=file->errhandler();
  return MPI_SUCCESS;
}

int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler){
  if (errhandler==nullptr){
    return MPI_ERR_ARG;
  } else if (file == MPI_FILE_NULL) {
    SMPI_default_File_Errhandler = errhandler;
    return MPI_SUCCESS;
  }
  file->set_errhandler(errhandler);
  return MPI_SUCCESS;
}

int PMPI_File_call_errhandler(MPI_File file,int errorcode){
  if (file == nullptr) {
    return MPI_ERR_WIN;
  }
  MPI_Errhandler err = file->errhandler();
  err->call(file, errorcode);
  simgrid::smpi::Errhandler::unref(err);
  return MPI_SUCCESS;
}
