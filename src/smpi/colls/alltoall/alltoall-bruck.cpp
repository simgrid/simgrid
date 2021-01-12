/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*****************************************************************************

 * Function: alltoall_bruck

 * Return: int

 * inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function realizes the alltoall operation using the bruck algorithm.

 * Author: MPICH / modified by Ahmad Faraj

 ****************************************************************************/

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{


int
alltoall__bruck(const void *send_buff, int send_count,
                MPI_Datatype send_type, void *recv_buff,
                int recv_count, MPI_Datatype recv_type,
                MPI_Comm comm)
{
  MPI_Status status;
  MPI_Aint extent;
  MPI_Datatype new_type;

  int i, src, dst, rank, num_procs, count, block, position;
  int pack_size, tag = COLL_TAG_ALLTOALL, pof2 = 1;

  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  num_procs = comm->size();
  rank = comm->rank();

  extent = recv_type->get_extent();

  unsigned char* tmp_buff = smpi_get_tmp_sendbuffer(num_procs * recv_count * extent);
  int* disps         = new int[num_procs];
  int* blocks_length = new int[num_procs];

  Request::sendrecv(send_ptr + rank * send_count * extent,
               (num_procs - rank) * send_count, send_type, rank, tag,
               recv_ptr, (num_procs - rank) * recv_count, recv_type, rank,
               tag, comm, &status);

  Request::sendrecv(send_ptr, rank * send_count, send_type, rank, tag,
               recv_ptr + (num_procs - rank) * recv_count * extent,
               rank * recv_count, recv_type, rank, tag, comm, &status);



  MPI_Pack_size(send_count * num_procs, send_type, comm, &pack_size);

  while (pof2 < num_procs) {
    dst = (rank + pof2) % num_procs;
    src = (rank - pof2 + num_procs) % num_procs;


    count = 0;
    for (block = 1; block < num_procs; block++)
      if (block & pof2) {
        blocks_length[count] = send_count;
        disps[count] = block * send_count;
        count++;
      }

    MPI_Type_indexed(count, blocks_length, disps, recv_type, &new_type);
    new_type->commit();

    position = 0;
    MPI_Pack(recv_buff, 1, new_type, tmp_buff, pack_size, &position, comm);

    Request::sendrecv(tmp_buff, position, MPI_PACKED, dst, tag, recv_buff, 1,
                 new_type, src, tag, comm, &status);
    Datatype::unref(new_type);

    pof2 *= 2;
  }

  delete[] disps;
  delete[] blocks_length;

  Request::sendrecv(recv_ptr + (rank + 1) * recv_count * extent,
               (num_procs - rank - 1) * recv_count, send_type,
               rank, tag, tmp_buff, (num_procs - rank - 1) * recv_count,
               recv_type, rank, tag, comm, &status);

  Request::sendrecv(recv_ptr, (rank + 1) * recv_count, send_type, rank, tag,
               tmp_buff + (num_procs - rank - 1) * recv_count * extent,
               (rank + 1) * recv_count, recv_type, rank, tag, comm, &status);


  for (i = 0; i < num_procs; i++)
    Request::sendrecv(tmp_buff + i * recv_count * extent, recv_count, send_type,
                 rank, tag,
                 recv_ptr + (num_procs - i - 1) * recv_count * extent,
                 recv_count, recv_type, rank, tag, comm, &status);

  smpi_free_tmp_buffer(tmp_buff);
  return MPI_SUCCESS;
}

}
}
