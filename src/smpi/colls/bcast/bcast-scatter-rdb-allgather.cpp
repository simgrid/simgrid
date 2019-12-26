/* Copyright (c) 2011-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include "smpi_status.hpp"

namespace simgrid{
namespace smpi{

static int scatter_for_bcast(
    int root,
    MPI_Comm comm,
    int nbytes,
    void *tmp_buf)
{
    MPI_Status status;
    int        rank, comm_size, src, dst;
    int        relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int scatter_size, curr_size, recv_size = 0, send_size;

    comm_size = comm->size();
    rank = comm->rank();
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* use long message algorithm: binomial tree scatter followed by an allgather */

    /* The scatter algorithm divides the buffer into nprocs pieces and
       scatters them among the processes. Root gets the first piece,
       root+1 gets the second piece, and so forth. Uses the same binomial
       tree algorithm as above. Ceiling division
       is used to compute the size of each piece. This means some
       processes may not get any data. For example if bufsize = 97 and
       nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
       scattered data is stored at the same offset in the buffer as it is
       on the root process. */

    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0; /* root starts with all the
                                                data */

    mask = 0x1;
    while (mask < comm_size)
    {
        if (relative_rank & mask)
        {
            src = rank - mask;
            if (src < 0) src += comm_size;
            recv_size = nbytes - relative_rank*scatter_size;
            /* recv_size is larger than what might actually be sent by the
               sender. We don't need compute the exact value because MPI
               allows you to post a larger recv.*/
            if (recv_size <= 0)
            {
                curr_size = 0; /* this process doesn't receive any data
                                  because of uneven division */
            }
            else
            {
                Request::recv(((char *)tmp_buf +
                                          relative_rank*scatter_size),
                                         recv_size, MPI_BYTE, src,
                                         COLL_TAG_BCAST, comm, &status);
                /* query actual size of data received */
                curr_size=Status::get_count(&status, MPI_BYTE);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down
       one. */

    mask >>= 1;
    while (mask > 0)
    {
        if (relative_rank + mask < comm_size)
        {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0)
            {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                Request::send(((char *)tmp_buf +
                                          scatter_size*(relative_rank+mask)),
                                         send_size, MPI_BYTE, dst,
                                         COLL_TAG_BCAST, comm);
                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

    return mpi_errno;
}


int bcast__scatter_rdb_allgather(
    void *buffer,
    int count,
    MPI_Datatype datatype,
    int root,
    MPI_Comm comm)
{
    MPI_Status status;
    int rank, comm_size, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int scatter_size, curr_size, recv_size = 0;
    int j, k, i, tmp_mask;
    bool is_contig, is_homogeneous;
    MPI_Aint type_size = 0, nbytes = 0;
    int relative_dst, dst_tree_root, my_tree_root, send_offset;
    int recv_offset, tree_root, nprocs_completed, offset;
    int position;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf;

    comm_size = comm->size();
    rank = comm->rank();
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    //if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    is_contig = ((datatype->flags() & DT_FLAG_CONTIGUOUS) != 0);

    is_homogeneous = true;

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        type_size=datatype->size();

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if (is_contig && is_homogeneous)
    {
        /* contiguous and homogeneous. no need to pack. */
        datatype->extent(&true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    }
    else
    {
      tmp_buf = new unsigned char[nbytes];

      /* TODO: Pipeline the packing and communication */
      position = 0;
      if (rank == root) {
        mpi_errno = datatype->pack(buffer, count, tmp_buf, nbytes, &position, comm);
        if (mpi_errno)
          xbt_die("crash while packing %d", mpi_errno);
      }
    }


    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */

    mpi_errno = scatter_for_bcast(root, comm,
                                  nbytes, tmp_buf);
    if (mpi_errno) {
      xbt_die("crash while scattering %d", mpi_errno);
    }

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (relative_rank*scatter_size) */
    curr_size = scatter_size < (nbytes - (relative_rank * scatter_size)) ? scatter_size :  (nbytes - (relative_rank * scatter_size)) ;
    if (curr_size < 0)
        curr_size = 0;

    /* medium size allgather and pof2 comm_size. use recurive doubling. */

    mask = 0x1;
    i = 0;
    while (mask < comm_size)
    {
        relative_dst = relative_rank ^ mask;

        dst = (relative_dst + root) % comm_size;

        /* find offset into send and recv buffers.
           zero out the least significant "i" bits of relative_rank and
           relative_dst to find root of src and dst
           subtrees. Use ranks of roots as index to send from
           and recv into  buffer */

        dst_tree_root = relative_dst >> i;
        dst_tree_root <<= i;

        my_tree_root = relative_rank >> i;
        my_tree_root <<= i;

        send_offset = my_tree_root * scatter_size;
        recv_offset = dst_tree_root * scatter_size;

        if (relative_dst < comm_size)
        {
            Request::sendrecv(((char *)tmp_buf + send_offset),
                                         curr_size, MPI_BYTE, dst, COLL_TAG_BCAST,
                                         ((char *)tmp_buf + recv_offset),
                                         (nbytes-recv_offset < 0 ? 0 : nbytes-recv_offset),
                                         MPI_BYTE, dst, COLL_TAG_BCAST, comm, &status);
            recv_size=Status::get_count(&status, MPI_BYTE);
            curr_size += recv_size;
        }

        /* if some processes in this process's subtree in this step
           did not have any destination process to communicate with
           because of non-power-of-two, we need to send them the
           data that they would normally have received from those
           processes. That is, the haves in this subtree must send to
           the havenots. We use a logarithmic recursive-halfing algorithm
           for this. */

        /* This part of the code will not currently be
           executed because we are not using recursive
           doubling for non power of two. Mark it as experimental
           so that it doesn't show up as red in the coverage tests. */

        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size)
        {
            nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
               subtree that have all the data. Send data to others
               in a tree fashion. First find root of current tree
               that is being divided into two. k is the number of
               least-significant bits in this process's rank that
               must be zeroed out to find the rank of the root */
            j = mask;
            k = 0;
            while (j)
            {
                j >>= 1;
                k++;
            }
            k--;

            offset = scatter_size * (my_tree_root + mask);
            tmp_mask = mask >> 1;

            while (tmp_mask)
            {
                relative_dst = relative_rank ^ tmp_mask;
                dst = (relative_dst + root) % comm_size;

                tree_root = relative_rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. */

                /* if (rank == 3) {
                   printf("rank %d, dst %d, root %d, nprocs_completed %d\n", relative_rank, relative_dst, tree_root, nprocs_completed);
                   fflush(stdout);
                   }*/

                if ((relative_dst > relative_rank) &&
                    (relative_rank < tree_root + nprocs_completed)
                    && (relative_dst >= tree_root + nprocs_completed))
                {

                    /* printf("Rank %d, send to %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                       fflush(stdout); */
                    Request::send(((char *)tmp_buf + offset),
                                             recv_size, MPI_BYTE, dst,
                                             COLL_TAG_BCAST, comm);
                    /* recv_size was set in the previous
                       receive. that's the amount of data to be
                       sent now. */
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((relative_dst < relative_rank) &&
                         (relative_dst < tree_root + nprocs_completed) &&
                         (relative_rank >= tree_root + nprocs_completed))
                {
                    /* printf("Rank %d waiting to recv from rank %d\n",
                       relative_rank, dst); */
                    Request::recv(((char *)tmp_buf + offset),
                                             nbytes - offset,
                                             MPI_BYTE, dst, COLL_TAG_BCAST,
                                             comm, &status);
                    /* nprocs_completed is also equal to the no. of processes
                       whose data we don't have */
                    recv_size=Status::get_count(&status, MPI_BYTE);
                    curr_size += recv_size;
                    /* printf("Rank %d, recv from %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                       fflush(stdout);*/
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }

    /* check that we received as much as we expected */
    /* recvd_size may not be accurate for packed heterogeneous data */
    if (is_homogeneous && curr_size != nbytes) {
      xbt_die("we didn't receive enough !");
    }

    if (not is_contig || not is_homogeneous) {
      if (rank != root) {
        position  = 0;
        mpi_errno = MPI_Unpack(tmp_buf, nbytes, &position, buffer, count, datatype, comm);
        if (mpi_errno)
          xbt_die("error when unpacking %d", mpi_errno);
      }
    }

fn_exit:
  /* delete[] static_cast<unsigned char*>(tmp_buf); */
  return mpi_errno;
}

}
}
