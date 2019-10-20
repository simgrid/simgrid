/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Copyright (c) 2001-2014, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 *
 */

#include "../colls_private.hpp"
#include <algorithm>

namespace simgrid{
namespace smpi{
int Coll_allreduce_mvapich2_rs::allreduce(const void *sendbuf,
                            void *recvbuf,
                            int count,
                            MPI_Datatype datatype,
                            MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int newrank = 0;
    int mask, pof2, i, send_idx, recv_idx, last_idx, send_cnt;
    int dst, is_commutative, rem, newdst, recv_cnt;
    MPI_Aint true_lb, true_extent, extent;

    if (count == 0) {
        return MPI_SUCCESS;
    }

    /* homogeneous */

    int comm_size =  comm->size();
    int rank = comm->rank();

    is_commutative = (op==MPI_OP_NULL || op->is_commutative());

    /* need to allocate temporary buffer to store incoming data */
    datatype->extent(&true_lb, &true_extent);
    extent = datatype->get_extent();

    unsigned char* tmp_buf_free = smpi_get_tmp_recvbuffer(count * std::max(extent, true_extent));

    /* adjust for potential negative lower bound in datatype */
    unsigned char* tmp_buf = tmp_buf_free - true_lb;

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno =
            Datatype::copy(sendbuf, count, datatype, recvbuf, count,
                           datatype);
    }

    /* find nearest power-of-two less than or equal to comm_size */
    for( pof2 = 1; pof2 <= comm_size; pof2 <<= 1 );
    pof2 >>=1;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {
            /* even */
            Request::send(recvbuf, count, datatype, rank + 1,
                                     COLL_TAG_ALLREDUCE, comm);

            /* temporarily set the rank to -1 so that this
               process does not participate in recursive
               doubling */
            newrank = -1;
        } else {
            /* odd */
            Request::recv(tmp_buf, count, datatype, rank - 1,
                                     COLL_TAG_ALLREDUCE, comm,
                                     MPI_STATUS_IGNORE);
            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
               if(op!=MPI_OP_NULL) op->apply( tmp_buf, recvbuf, &count, datatype);
                /* change the rank */
                newrank = rank / 2;
        }
    } else {                /* rank >= 2*rem */
        newrank = rank - rem;
    }

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if (newrank != -1) {
        if (/*(HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||*/ (count < pof2)) {  /* use recursive doubling */
            mask = 0x1;
            while (mask < pof2) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

                /* Send the most current data, which is in recvbuf. Recv
                   into tmp_buf */
                Request::sendrecv(recvbuf, count, datatype,
                                             dst, COLL_TAG_ALLREDUCE,
                                             tmp_buf, count, datatype, dst,
                                             COLL_TAG_ALLREDUCE, comm,
                                             MPI_STATUS_IGNORE);

                /* tmp_buf contains data received in this step.
                   recvbuf contains data accumulated so far */

                if (is_commutative || (dst < rank)) {
                    /* op is commutative OR the order is already right */
                     if(op!=MPI_OP_NULL) op->apply( tmp_buf, recvbuf, &count, datatype);
                } else {
                    /* op is noncommutative and the order is not right */
                    if(op!=MPI_OP_NULL) op->apply( recvbuf, tmp_buf, &count, datatype);
                    /* copy result back into recvbuf */
                    mpi_errno = Datatype::copy(tmp_buf, count, datatype,
                                               recvbuf, count, datatype);
                }
                mask <<= 1;
            }
        } else {

            /* do a reduce-scatter followed by allgather */

            /* for the reduce-scatter, calculate the count that
               each process receives and the displacement within
               the buffer */
            int* cnts  = new int[pof2];
            int* disps = new int[pof2];

            for (i = 0; i < (pof2 - 1); i++) {
                cnts[i] = count / pof2;
            }
            cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

            disps[0] = 0;
            for (i = 1; i < pof2; i++) {
                disps[i] = disps[i - 1] + cnts[i - 1];
            }

            mask = 0x1;
            send_idx = recv_idx = 0;
            last_idx = pof2;
            while (mask < pof2) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    send_idx = recv_idx + pof2 / (mask * 2);
                    for (i = send_idx; i < last_idx; i++)
                        send_cnt += cnts[i];
                    for (i = recv_idx; i < send_idx; i++)
                        recv_cnt += cnts[i];
                } else {
                    recv_idx = send_idx + pof2 / (mask * 2);
                    for (i = send_idx; i < recv_idx; i++)
                        send_cnt += cnts[i];
                    for (i = recv_idx; i < last_idx; i++)
                        recv_cnt += cnts[i];
                }

                /* Send data from recvbuf. Recv into tmp_buf */
                Request::sendrecv(static_cast<char*>(recvbuf) + disps[send_idx] * extent, send_cnt, datatype, dst,
                                  COLL_TAG_ALLREDUCE, tmp_buf + disps[recv_idx] * extent, recv_cnt, datatype, dst,
                                  COLL_TAG_ALLREDUCE, comm, MPI_STATUS_IGNORE);

                /* tmp_buf contains data received in this step.
                   recvbuf contains data accumulated so far */

                /* This algorithm is used only for predefined ops
                   and predefined ops are always commutative. */

                if (op != MPI_OP_NULL)
                  op->apply(tmp_buf + disps[recv_idx] * extent, static_cast<char*>(recvbuf) + disps[recv_idx] * extent,
                            &recv_cnt, datatype);

                /* update send_idx for next iteration */
                send_idx = recv_idx;
                mask <<= 1;

                /* update last_idx, but not in last iteration
                   because the value is needed in the allgather
                   step below. */
                if (mask < pof2)
                    last_idx = recv_idx + pof2 / mask;
            }

            /* now do the allgather */

            mask >>= 1;
            while (mask > 0) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    /* update last_idx except on first iteration */
                    if (mask != pof2 / 2) {
                        last_idx = last_idx + pof2 / (mask * 2);
                    }

                    recv_idx = send_idx + pof2 / (mask * 2);
                    for (i = send_idx; i < recv_idx; i++) {
                        send_cnt += cnts[i];
                    }
                    for (i = recv_idx; i < last_idx; i++) {
                        recv_cnt += cnts[i];
                    }
                } else {
                    recv_idx = send_idx - pof2 / (mask * 2);
                    for (i = send_idx; i < last_idx; i++) {
                        send_cnt += cnts[i];
                    }
                    for (i = recv_idx; i < send_idx; i++) {
                        recv_cnt += cnts[i];
                    }
                }

               Request::sendrecv((char *) recvbuf +
                                             disps[send_idx] * extent,
                                             send_cnt, datatype,
                                             dst, COLL_TAG_ALLREDUCE,
                                             (char *) recvbuf +
                                             disps[recv_idx] * extent,
                                             recv_cnt, datatype, dst,
                                             COLL_TAG_ALLREDUCE, comm,
                                             MPI_STATUS_IGNORE);
                if (newrank > newdst) {
                    send_idx = recv_idx;
                }

                mask >>= 1;
            }
            delete[] disps;
            delete[] cnts;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) {     /* odd */
            Request::send(recvbuf, count,
                                     datatype, rank - 1,
                                     COLL_TAG_ALLREDUCE, comm);
        } else {            /* even */
            Request::recv(recvbuf, count,
                                  datatype, rank + 1,
                                  COLL_TAG_ALLREDUCE, comm,
                                  MPI_STATUS_IGNORE);
        }
    }
    smpi_free_tmp_buffer(tmp_buf_free);
    return (mpi_errno);

}

}
}
