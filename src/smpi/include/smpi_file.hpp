/* Copyright (c) 2010-2020. The SimGrid Team.
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
class File{
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
  ~File();
  int size();
  int get_position(MPI_Offset* offset);
  int get_position_shared(MPI_Offset* offset);
  int flags();
  MPI_Comm comm();
  int sync();
  int seek(MPI_Offset offset, int whence);
  int seek_shared(MPI_Offset offset, int whence);
  int set_view(MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char* datarep, const Info* info);
  int get_view(MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep);
  MPI_Info info();
  void set_info( MPI_Info info);
  static int read(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int read_shared(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int read_ordered(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int write(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int write_shared(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int write_ordered(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  template <int (*T)(MPI_File, void *, int, MPI_Datatype, MPI_Status *)> int op_all(void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
  static int close(MPI_File *fh);
  static int del(const char* filename, const Info* info);
  MPI_Errhandler errhandler();
  void set_errhandler( MPI_Errhandler errhandler);
};

  /* Read_all, Write_all : loosely based on */
  /* @article{Thakur:1996:ETM:245875.245879,*/
  /* author = {Thakur, Rajeev and Choudhary, Alok},*/
  /* title = {An Extended Two-phase Method for Accessing Sections of Out-of-core Arrays},*/
  /* journal = {Sci. Program.},*/
  /* issue_date = {Winter 1996},*/
  /* pages = {301--317},*/
  /* }*/ 
  template <int (*T)(MPI_File, void *, int, MPI_Datatype, MPI_Status *)>
  int File::op_all(void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
    //get min and max offsets from everyone.
    int size =  comm_->size();
    int rank = comm_-> rank();
    MPI_Offset min_offset = file_->tell();
    MPI_Offset max_offset = min_offset + count * datatype->get_extent();//cheating, as we don't care about exact data location, we can skip extent
    MPI_Offset* min_offsets = new MPI_Offset[size];
    MPI_Offset* max_offsets = new MPI_Offset[size];
    simgrid::smpi::colls::allgather(&min_offset, 1, MPI_OFFSET, min_offsets, 1, MPI_OFFSET, comm_);
    simgrid::smpi::colls::allgather(&max_offset, 1, MPI_OFFSET, max_offsets, 1, MPI_OFFSET, comm_);
    MPI_Offset min=min_offset;
    MPI_Offset max=max_offset;
    MPI_Offset tot= 0;
    int empty=1;
    for(int i=0;i<size;i++){
      if(min_offsets[i]!=max_offsets[i])
        empty=0;
      tot+=(max_offsets[i]-min_offsets[i]);
      if(min_offsets[i]<min)
        min=min_offsets[i];
      if(max_offsets[i]>max)
        max=max_offsets[i];
    }
    
    XBT_CDEBUG(smpi_pmpi, "my offsets to read : %lld:%lld, global min and max %lld:%lld", min_offset, max_offset, min, max);
    if(empty==1){
      delete[] min_offsets;
      delete[] max_offsets;
      status->count=0;
      return MPI_SUCCESS;
    }
    MPI_Offset total = max-min;
    if(total==tot && (datatype->flags() & DT_FLAG_CONTIGUOUS)){
      delete[] min_offsets;
      delete[] max_offsets;
      //contiguous. Just have each proc perform its read
      if(status != MPI_STATUS_IGNORE)
        status->count=count * datatype->size();
      return T(this,buf,count,datatype, status);
    }

    //Interleaved case : How much do I need to read, and whom to send it ?
    MPI_Offset my_chunk_start=(max-min+1)/size*rank;
    MPI_Offset my_chunk_end=((max-min+1)/size*(rank+1));
    XBT_CDEBUG(smpi_pmpi, "my chunks to read : %lld:%lld", my_chunk_start, my_chunk_end);
    int* send_sizes = new int[size];
    int* recv_sizes = new int[size];
    int* send_disps = new int[size];
    int* recv_disps = new int[size];
    int total_sent=0;
    for(int i=0;i<size;i++){
      send_sizes[i]=0;
      send_disps[i]=0;//cheat to avoid issues when send>recv as we use recv buffer
      if((my_chunk_start>=min_offsets[i] && my_chunk_start < max_offsets[i])||
          ((my_chunk_end<=max_offsets[i]) && my_chunk_end> min_offsets[i])){
        send_sizes[i]=(std::min(max_offsets[i]-1, my_chunk_end-1)-std::max(min_offsets[i], my_chunk_start));
        // store min and max offset to actually read
        min_offset=std::min(min_offset, min_offsets[i]);
        total_sent+=send_sizes[i];
        XBT_CDEBUG(smpi_pmpi, "will have to send %d bytes to %d", send_sizes[i], i);
      }
    }
    min_offset=std::max(min_offset, my_chunk_start);

    //merge the ranges of every process
    std::vector<std::pair<MPI_Offset, MPI_Offset>> ranges;
    for(int i=0; i<size; ++i)
      ranges.push_back(std::make_pair(min_offsets[i],max_offsets[i]));
    std::sort(ranges.begin(), ranges.end());
    std::vector<std::pair<MPI_Offset, MPI_Offset>> chunks;
    chunks.push_back(ranges[0]);

    unsigned int nchunks=0;
    unsigned int i=1;
    while(i < ranges.size()){
      if(ranges[i].second>chunks[nchunks].second){
        // else range included - ignore
        if(ranges[i].first>chunks[nchunks].second){
          //new disjoint range
          chunks.push_back(ranges[i]);
          nchunks++;
        } else {
          //merge ranges
          chunks[nchunks].second=ranges[i].second;
        }
      }
      i++;
    }
    //what do I need to read ?
    MPI_Offset totreads=0;
    for(i=0; i<chunks.size();i++){
      if(chunks[i].second < my_chunk_start)
        continue;
      else if (chunks[i].first > my_chunk_end)
        continue;
      else
        totreads += (std::min(chunks[i].second, my_chunk_end-1)-std::max(chunks[i].first, my_chunk_start));
    }
    XBT_CDEBUG(smpi_pmpi, "will have to access %lld from my chunk", totreads);

    unsigned char* sendbuf = smpi_get_tmp_sendbuffer(total_sent);

    if(totreads>0){
      seek(min_offset, MPI_SEEK_SET);
      T(this,sendbuf,totreads/datatype->size(),datatype, status);
    }
    simgrid::smpi::colls::alltoall(send_sizes, 1, MPI_INT, recv_sizes, 1, MPI_INT, comm_);
    int total_recv=0;
    for(int i=0;i<size;i++){
      recv_disps[i]=total_recv;
      total_recv+=recv_sizes[i];
    }
    //Set buf value to avoid copying dumb data
    simgrid::smpi::colls::alltoallv(sendbuf, send_sizes, send_disps, MPI_BYTE, buf, recv_sizes, recv_disps, MPI_BYTE,
                                    comm_);
    if(status!=MPI_STATUS_IGNORE)
      status->count=count * datatype->size();
    smpi_free_tmp_buffer(sendbuf);
    delete[] send_sizes;
    delete[] recv_sizes;
    delete[] send_disps;
    delete[] recv_disps;
    delete[] min_offsets;
    delete[] max_offsets;
    return MPI_SUCCESS;
  }
}
}

#endif
