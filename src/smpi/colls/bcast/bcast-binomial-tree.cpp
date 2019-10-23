/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

/*****************************************************************************

Copyright (c) 2006, Ahmad Faraj & Xin Yuan,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the Florida State University nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  *************************************************************************
  *     Any results obtained from executing this software require the     *
  *     acknowledgment and citation of the software and its owners.       *
  *     The full citation is given below:                                 *
  *                                                                       *
  *     A. Faraj and X. Yuan. "Automatic Generation and Tuning of MPI     *
  *     Collective Communication Routines." The 19th ACM International    *
  *     Conference on Supercomputing (ICS), Cambridge, Massachusetts,     *
  *     June 20-22, 2005.                                                 *
  *************************************************************************

*****************************************************************************/

/*****************************************************************************

 * Function: bcast_binomial_tree

 * Return: int

 * Inputs:
    buff: send input buffer
    count: number of elements to send
    data_type: data type of elements being sent
    root: source of data
    comm: communicator

 * Descrp: broadcasts using a bionomial tree.

 * Author: MPIH / modified by Ahmad Faraj

 ****************************************************************************/
namespace simgrid{
namespace smpi{
int
Coll_bcast_binomial_tree::bcast(void *buff, int count,
                                    MPI_Datatype data_type, int root,
                                    MPI_Comm comm)
{
  int src, dst, rank, num_procs, mask, relative_rank;
  int tag = COLL_TAG_BCAST;

  rank = comm->rank();
  num_procs = comm->size();

  relative_rank = (rank >= root) ? rank - root : rank - root + num_procs;

  mask = 0x1;
  while (mask < num_procs) {
    if (relative_rank & mask) {
      src = rank - mask;
      if (src < 0)
        src += num_procs;
      Request::recv(buff, count, data_type, src, tag, comm, MPI_STATUS_IGNORE);
      break;
    }
    mask <<= 1;
  }

  mask >>= 1;
  while (mask > 0) {
    if (relative_rank + mask < num_procs) {
      dst = rank + mask;
      if (dst >= num_procs)
        dst -= num_procs;
      Request::send(buff, count, data_type, dst, tag, comm);
    }
    mask >>= 1;
  }

  return MPI_SUCCESS;
}

}
}
