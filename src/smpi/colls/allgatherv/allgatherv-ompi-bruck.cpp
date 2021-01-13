/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      University of Houston. All rights reserved.
 *
 * Additional copyrights may follow
 */

#include "../colls_private.hpp"
/*
 * ompi_coll_tuned_allgatherv_intra_bruck
 *
 * Function:     allgather using O(log(N)) steps.
 * Accepts:      Same arguments as MPI_Allgather
 * Returns:      MPI_SUCCESS or error code
 *
 * Description:  Variation to All-to-all algorithm described by Bruck et al.in
 *               "Efficient Algorithms for All-to-all Communications
 *                in Multiport Message-Passing Systems"
 * Note:         Unlike in case of allgather implementation, we relay on
 *               indexed datatype to select buffers appropriately.
 *               The only additional memory requirement is for creation of
 *               temporary datatypes.
 * Example on 7 nodes (memory lay out need not be in-order)
 *   Initial set up:
 *    #     0      1      2      3      4      5      6
 *         [0]    [ ]    [ ]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [1]    [ ]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [2]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [3]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [4]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [ ]    [5]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [ ]    [ ]    [6]
 *   Step 0: send message to (rank - 2^0), receive message from (rank + 2^0)
 *    #     0      1      2      3      4      5      6
 *         [0]    [ ]    [ ]    [ ]    [ ]    [ ]    [0]
 *         [1]    [1]    [ ]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [2]    [2]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [3]    [3]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [4]    [4]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [5]    [5]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [ ]    [6]    [6]
 *   Step 1: send message to (rank - 2^1), receive message from (rank + 2^1).
 *           message contains all blocks from (rank) .. (rank + 2^2) with
 *           wrap around.
 *    #     0      1      2      3      4      5      6
 *         [0]    [ ]    [ ]    [ ]    [0]    [0]    [0]
 *         [1]    [1]    [ ]    [ ]    [ ]    [1]    [1]
 *         [2]    [2]    [2]    [ ]    [ ]    [ ]    [2]
 *         [3]    [3]    [3]    [3]    [ ]    [ ]    [ ]
 *         [ ]    [4]    [4]    [4]    [4]    [ ]    [ ]
 *         [ ]    [ ]    [5]    [5]    [5]    [5]    [ ]
 *         [ ]    [ ]    [ ]    [6]    [6]    [6]    [6]
 *   Step 2: send message to (rank - 2^2), receive message from (rank + 2^2).
 *           message size is "all remaining blocks"
 *    #     0      1      2      3      4      5      6
 *         [0]    [0]    [0]    [0]    [0]    [0]    [0]
 *         [1]    [1]    [1]    [1]    [1]    [1]    [1]
 *         [2]    [2]    [2]    [2]    [2]    [2]    [2]
 *         [3]    [3]    [3]    [3]    [3]    [3]    [3]
 *         [4]    [4]    [4]    [4]    [4]    [4]    [4]
 *         [5]    [5]    [5]    [5]    [5]    [5]    [5]
 *         [6]    [6]    [6]    [6]    [6]    [6]    [6]
 */

namespace simgrid {
namespace smpi {

int allgatherv__ompi_bruck(const void *sbuf, int scount,
                           MPI_Datatype sdtype,
                           void *rbuf, const int *rcounts,
                           const int *rdispls,
                           MPI_Datatype rdtype,
                           MPI_Comm comm)
{
   int sendto, recvfrom, blockcount, i;
   unsigned int distance;
   ptrdiff_t slb, rlb, sext, rext;
   char *tmpsend = nullptr, *tmprecv = nullptr;
   MPI_Datatype new_rdtype = MPI_DATATYPE_NULL, new_sdtype = MPI_DATATYPE_NULL;

   unsigned int size = comm->size();
   unsigned int rank = comm->rank();

   XBT_DEBUG("coll:tuned:allgather_ompi_bruck rank %u", rank);

   sdtype->extent(&slb, &sext);

   rdtype->extent(&rlb, &rext);

   /* Initialization step:
      - if send buffer is not MPI_IN_PLACE, copy send buffer to block rank of
        the receive buffer.
   */
   tmprecv = (char*) rbuf + rdispls[rank] * rext;
   if (MPI_IN_PLACE != sbuf) {
      tmpsend = (char*) sbuf;
      Datatype::copy(tmpsend, scount, sdtype,
                            tmprecv, rcounts[rank], rdtype);
   }

   /* Communication step:
      At every step i, rank r:
      - doubles the distance
      - sends message with blockcount blocks, (rbuf[rank] .. rbuf[rank + 2^i])
        to rank (r - distance)
      - receives message of blockcount blocks,
        (rbuf[r + distance] ... rbuf[(r+distance) + 2^i]) from
        rank (r + distance)
      - blockcount doubles until the last step when only the remaining data is
      exchanged.
   */
   int* new_rcounts = new int[4 * size];
   int* new_rdispls = new_rcounts + size;
   int* new_scounts = new_rdispls + size;
   int* new_sdispls = new_scounts + size;

   for (distance = 1; distance < size; distance<<=1) {

      recvfrom = (rank + distance) % size;
      sendto = (rank - distance + size) % size;

      if (distance <= (size >> 1)) {
         blockcount = distance;
      } else {
         blockcount = size - distance;
      }

      /* create send and receive datatypes */
      for (i = 0; i < blockcount; i++) {
          const int tmp_srank = (rank + i) % size;
          const int tmp_rrank = (recvfrom + i) % size;
          new_scounts[i] = rcounts[tmp_srank];
          new_sdispls[i] = rdispls[tmp_srank];
          new_rcounts[i] = rcounts[tmp_rrank];
          new_rdispls[i] = rdispls[tmp_rrank];
      }
      Datatype::create_indexed(blockcount, new_scounts, new_sdispls,
                                    rdtype, &new_sdtype);
      Datatype::create_indexed(blockcount, new_rcounts, new_rdispls,
                                    rdtype, &new_rdtype);

      new_sdtype->commit();
      new_rdtype->commit();

      /* Sendreceive */
      Request::sendrecv(rbuf, 1, new_sdtype, sendto,
                                     COLL_TAG_ALLGATHERV,
                                     rbuf, 1, new_rdtype, recvfrom,
                                     COLL_TAG_ALLGATHERV,
                                     comm, MPI_STATUS_IGNORE);
      Datatype::unref(new_sdtype);
      Datatype::unref(new_rdtype);

   }

   delete[] new_rcounts;

   return MPI_SUCCESS;

}


}
}
