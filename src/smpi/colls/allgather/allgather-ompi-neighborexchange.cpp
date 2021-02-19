/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * ompi_coll_tuned_allgather_intra_neighborexchange
 *
 * Function:     allgather using N/2 steps (O(N))
 * Accepts:      Same arguments as MPI_Allgather
 * Returns:      MPI_SUCCESS or error code
 *
 * Description:  Neighbor Exchange algorithm for allgather.
 *               Described by Chen et.al. in
 *               "Performance Evaluation of Allgather Algorithms on
 *                Terascale Linux Cluster with Fast Ethernet",
 *               Proceedings of the Eighth International Conference on
 *               High-Performance Computing inn Asia-Pacific Region
 *               (HPCASIA'05), 2005
 *
 *               Rank r exchanges message with one of its neighbors and
 *               forwards the data further in the next step.
 *
 *               No additional memory requirements.
 *
 * Limitations:  Algorithm works only on even number of processes.
 *               For odd number of processes we switch to ring algorithm.
 *
 * Example on 6 nodes:
 *  Initial state
 *    #     0      1      2      3      4      5
 *         [0]    [ ]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [1]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [2]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [3]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [4]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [ ]    [5]
 *   Step 0:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [ ]    [ ]    [ ]    [ ]
 *         [1]    [1]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [2]    [2]    [ ]    [ ]
 *         [ ]    [ ]    [3]    [3]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [4]    [4]
 *         [ ]    [ ]    [ ]    [ ]    [5]    [5]
 *   Step 1:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [0]    [ ]    [ ]    [0]
 *         [1]    [1]    [1]    [ ]    [ ]    [1]
 *         [ ]    [2]    [2]    [2]    [2]    [ ]
 *         [ ]    [3]    [3]    [3]    [3]    [ ]
 *         [4]    [ ]    [ ]    [4]    [4]    [4]
 *         [5]    [ ]    [ ]    [5]    [5]    [5]
 *   Step 2:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [0]    [0]    [0]    [0]
 *         [1]    [1]    [1]    [1]    [1]    [1]
 *         [2]    [2]    [2]    [2]    [2]    [2]
 *         [3]    [3]    [3]    [3]    [3]    [3]
 *         [4]    [4]    [4]    [4]    [4]    [4]
 *         [5]    [5]    [5]    [5]    [5]    [5]
 */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{

int
allgather__ompi_neighborexchange(const void *sbuf, int scount,
                                                 MPI_Datatype sdtype,
                                                 void* rbuf, int rcount,
                                                 MPI_Datatype rdtype,
                                                 MPI_Comm comm
)
{
   int line = -1;
   int rank, size;
   int neighbor[2], offset_at_step[2], recv_data_from[2], send_data_from;
   int i, even_rank;
   int err = 0;
   ptrdiff_t slb, rlb, sext, rext;
   char *tmpsend = nullptr, *tmprecv = nullptr;

   size = comm->size();
   rank = comm->rank();

   if (size % 2) {
      XBT_INFO(
                   "coll:tuned:allgather_intra_neighborexchange: odd size %d, switching to ring algorithm",
                   size);
      return allgather__ring(sbuf, scount, sdtype,
                             rbuf, rcount, rdtype,
                             comm);
   }

   XBT_DEBUG(
                "coll:tuned:allgather_intra_neighborexchange rank %d", rank);

   err = sdtype->extent(&slb, &sext);
   if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

   err = rdtype->extent(&rlb, &rext);
   if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

   /* Initialization step:
      - if send buffer is not MPI_IN_PLACE, copy send buffer to appropriate block
        of receive buffer
   */
   tmprecv = (char*) rbuf + rank * rcount * rext;
   if (MPI_IN_PLACE != sbuf) {
      tmpsend = (char*) sbuf;
      Datatype::copy (tmpsend, scount, sdtype, tmprecv, rcount, rdtype);
   }

   /* Determine neighbors, order in which blocks will arrive, etc. */
   even_rank = not(rank % 2);
   if (even_rank) {
      neighbor[0] = (rank + 1) % size;
      neighbor[1] = (rank - 1 + size) % size;
      recv_data_from[0] = rank;
      recv_data_from[1] = rank;
      offset_at_step[0] = (+2);
      offset_at_step[1] = (-2);
   } else {
      neighbor[0] = (rank - 1 + size) % size;
      neighbor[1] = (rank + 1) % size;
      recv_data_from[0] = neighbor[0];
      recv_data_from[1] = neighbor[0];
      offset_at_step[0] = (-2);
      offset_at_step[1] = (+2);
   }

   /* Communication loop:
      - First step is special: exchange a single block with neighbor[0].
      - Rest of the steps:
        update recv_data_from according to offset, and
        exchange two blocks with appropriate neighbor.
        the send location becomes previous receive location.
   */
   tmprecv = (char*)rbuf + neighbor[0] * rcount * rext;
   tmpsend = (char*)rbuf + rank * rcount * rext;
   /* Sendreceive */
   Request::sendrecv(tmpsend, rcount, rdtype, neighbor[0],
                                  COLL_TAG_ALLGATHER,
                                  tmprecv, rcount, rdtype, neighbor[0],
                                  COLL_TAG_ALLGATHER,
                                  comm, MPI_STATUS_IGNORE);

   /* Determine initial sending location */
   if (even_rank) {
      send_data_from = rank;
   } else {
      send_data_from = recv_data_from[0];
   }

   for (i = 1; i < (size / 2); i++) {
      const int i_parity = i % 2;
      recv_data_from[i_parity] =
         (recv_data_from[i_parity] + offset_at_step[i_parity] + size) % size;

      tmprecv = (char*)rbuf + recv_data_from[i_parity] * rcount * rext;
      tmpsend = (char*)rbuf + send_data_from * rcount * rext;

      /* Sendreceive */
      Request::sendrecv(tmpsend, 2 * rcount, rdtype,
                                     neighbor[i_parity],
                                     COLL_TAG_ALLGATHER,
                                     tmprecv, 2 * rcount, rdtype,
                                     neighbor[i_parity],
                                     COLL_TAG_ALLGATHER,
                                     comm, MPI_STATUS_IGNORE);

      send_data_from = recv_data_from[i_parity];
   }

   return MPI_SUCCESS;

 err_hndl:
   XBT_DEBUG( "%s:%4d\tError occurred %d, rank %2d",
                __FILE__, line, err, rank);
   return err;
}


}
}
