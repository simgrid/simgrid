/* Copyright (c) 2010-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_FILE_HPP_INCLUDED
#define SMPI_FILE_HPP_INCLUDED
#include "simgrid/plugins/file_system.h"
#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_info.hpp"
#include  <algorithm>

XBT_LOG_EXTERNAL_CATEGORY(smpi_pmpi);

namespace simgrid{
namespace smpi{
class File : public F2C{
  MPI_Comm comm_;
  int flags_;
  simgrid::s4u::File* file_;
  MPI_Info info_;
  MPI_Offset* shared_file_pointer_;
  s4u::MutexPtr shared_mutex_;
  MPI_Win win_;
  char* list_;
  MPI_Errhandler errhandler_;
  MPI_Datatype etype_;
  MPI_Datatype filetype_;
  std::string datarep_;

  public:
  File(MPI_Comm comm, const char *filename, int amode, MPI_Info info);
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  ~File() override;
  int size() const;
  int get_position(MPI_Offset* offset) const;
  int get_position_shared(MPI_Offset* offset) const;
  int flags() const;
  MPI_Comm comm() const;
  std::string name() const override {return file_ ? std::string("MPI_File: ")+ std::string(file_->get_path()): std::string("MPI_File");}

  int sync();
  int seek(MPI_Offset offset, int whence);
  int seek_shared(MPI_Offset offset, int whence);
  int set_view(MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char* datarep, const Info* info);
  int get_view(MPI_Offset* disp, MPI_Datatype* etype, MPI_Datatype* filetype, char* datarep) const;
  MPI_Info info();
  void set_info( MPI_Info info);
  void set_size(int size);
  static int read(MPI_File fh, void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int read_shared(MPI_File fh, void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int read_ordered(MPI_File fh, void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int write(MPI_File fh, void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int write_shared(MPI_File fh, const void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int write_ordered(MPI_File fh, const void* buf, int count, const Datatype* datatype, MPI_Status* status);
  template <int (*T)(MPI_File, void*, int, const Datatype*, MPI_Status*)>
  int op_all(void* buf, int count, const Datatype* datatype, MPI_Status* status);
  static int close(MPI_File *fh);
  static int del(const char* filename, const Info* info);
  MPI_Errhandler errhandler();
  void set_errhandler( MPI_Errhandler errhandler);
  static File* f2c(int id);
};

/* Read_all, Write_all : loosely based on */
/* @article{Thakur:1996:ETM:245875.245879,*/
/* author = {Thakur, Rajeev and Choudhary, Alok},*/
/* title = {An Extended Two-phase Method for Accessing Sections of Out-of-core Arrays},*/
/* journal = {Sci. Program.},*/
/* issue_date = {Winter 1996},*/
/* pages = {301--317},*/
/* }*/
template <int (*T)(MPI_File, void*, int, const Datatype*, MPI_Status*)>
int File::op_all(void* buf, int count, const Datatype* datatype, MPI_Status* status)
{
  // get min and max offsets from everyone.
  int size              = comm_->size();
  int rank              = comm_->rank();
  MPI_Offset min_offset = file_->tell();
  MPI_Offset max_offset =
      min_offset +
      count * datatype->get_extent(); // cheating, as we don't care about exact data location, we can skip extent
  std::vector<MPI_Offset> min_offsets(size);
  std::vector<MPI_Offset> max_offsets(size);
  simgrid::smpi::colls::allgather(&min_offset, 1, MPI_OFFSET, min_offsets.data(), 1, MPI_OFFSET, comm_);
  simgrid::smpi::colls::allgather(&max_offset, 1, MPI_OFFSET, max_offsets.data(), 1, MPI_OFFSET, comm_);
  MPI_Offset min = min_offset;
  MPI_Offset max = max_offset;
  MPI_Offset tot = 0;
  int empty      = 1;
  for (int i = 0; i < size; i++) {
    if (min_offsets[i] != max_offsets[i])
      empty = 0;
    tot += (max_offsets[i] - min_offsets[i]);
    if (min_offsets[i] < min)
      min = min_offsets[i];
    if (max_offsets[i] > max)
      max = max_offsets[i];
  }

  XBT_CDEBUG(smpi_pmpi, "my offsets to read : %lld:%lld, global min and max %lld:%lld", min_offset, max_offset, min,
             max);
  if (empty == 1) {
    if (status != MPI_STATUS_IGNORE)
      status->count = 0;
    return MPI_SUCCESS;
  }
  MPI_Offset total = max - min;
  if (total == tot && (datatype->flags() & DT_FLAG_CONTIGUOUS)) {
    // contiguous. Just have each proc perform its read
    if (status != MPI_STATUS_IGNORE)
      status->count = count * datatype->size();
    return T(this, buf, count, datatype, status);
  }

  // Interleaved case : How much do I need to read, and whom to send it ?
  MPI_Offset my_chunk_start = (max - min + 1) / size * rank;
  MPI_Offset my_chunk_end   = ((max - min + 1) / size * (rank + 1));
  XBT_CDEBUG(smpi_pmpi, "my chunks to read : %lld:%lld", my_chunk_start, my_chunk_end);
  std::vector<int> send_sizes(size);
  std::vector<int> recv_sizes(size);
  std::vector<int> send_disps(size);
  std::vector<int> recv_disps(size);
  int total_sent  = 0;
  for (int i = 0; i < size; i++) {
    send_sizes[i] = 0;
    send_disps[i] = 0; // cheat to avoid issues when send>recv as we use recv buffer
    if ((my_chunk_start >= min_offsets[i] && my_chunk_start < max_offsets[i]) ||
        ((my_chunk_end <= max_offsets[i]) && my_chunk_end > min_offsets[i])) {
      send_sizes[i] = (std::min(max_offsets[i] - 1, my_chunk_end - 1) - std::max(min_offsets[i], my_chunk_start));
      // store min and max offset to actually read
      min_offset = std::min(min_offset, min_offsets[i]);
      total_sent += send_sizes[i];
      XBT_CDEBUG(smpi_pmpi, "will have to send %d bytes to %d", send_sizes[i], i);
    }
  }
  min_offset = std::max(min_offset, my_chunk_start);

  // merge the ranges of every process
  std::vector<std::pair<MPI_Offset, MPI_Offset>> ranges;
  for (int i = 0; i < size; ++i)
    ranges.emplace_back(min_offsets[i], max_offsets[i]);
  std::sort(ranges.begin(), ranges.end());
  std::vector<std::pair<MPI_Offset, MPI_Offset>> chunks;
  chunks.push_back(ranges[0]);

  unsigned int nchunks = 0;
  for (unsigned i = 1; i < ranges.size(); i++) {
    if (ranges[i].second > chunks[nchunks].second) {
      // else range included - ignore
      if (ranges[i].first > chunks[nchunks].second) {
        // new disjoint range
        chunks.push_back(ranges[i]);
        nchunks++;
      } else {
        // merge ranges
        chunks[nchunks].second = ranges[i].second;
      }
    }
  }
  // what do I need to read ?
  MPI_Offset totreads = 0;
  for (auto const& chunk : chunks) {
    if (chunk.second < my_chunk_start)
      continue;
    else if (chunk.first > my_chunk_end)
      continue;
    else
      totreads += (std::min(chunk.second, my_chunk_end - 1) - std::max(chunk.first, my_chunk_start));
  }
  XBT_CDEBUG(smpi_pmpi, "will have to access %lld from my chunk", totreads);

  unsigned char* sendbuf = smpi_get_tmp_sendbuffer(total_sent);

  if (totreads > 0) {
    seek(min_offset, MPI_SEEK_SET);
    T(this, sendbuf, totreads / datatype->size(), datatype, status);
  }
  simgrid::smpi::colls::alltoall(send_sizes.data(), 1, MPI_INT, recv_sizes.data(), 1, MPI_INT, comm_);
  int total_recv = 0;
  for (int i = 0; i < size; i++) {
    recv_disps[i] = total_recv;
    total_recv += recv_sizes[i];
  }
  // Set buf value to avoid copying dumb data
  simgrid::smpi::colls::alltoallv(sendbuf, send_sizes.data(), send_disps.data(), MPI_BYTE, buf, recv_sizes.data(),
                                  recv_disps.data(), MPI_BYTE, comm_);
  if (status != MPI_STATUS_IGNORE)
    status->count = count * datatype->size();
  smpi_free_tmp_buffer(sendbuf);
  return MPI_SUCCESS;
}
}
}

#endif
