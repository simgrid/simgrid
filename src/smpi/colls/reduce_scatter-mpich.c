/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

static inline int MPIU_Mirror_permutation(unsigned int x, int bits)
{
    /* a mask for the high order bits that should be copied as-is */
    int high_mask = ~((0x1 << bits) - 1);
    int retval = x & high_mask;
    int i;

    for (i = 0; i < bits; ++i) {
        unsigned int bitval = (x & (0x1 << i)) >> i; /* 0x1 or 0x0 */
        retval |= bitval << ((bits - i) - 1);
    }

    return retval;
}


int smpi_coll_tuned_reduce_scatter_mpich_pair(void *sendbuf, void *recvbuf, int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb; 
    int  *disps;
    void *tmp_recvbuf;
    int mpi_errno = MPI_SUCCESS;
    int total_count, dst, src;
    int is_commutative;
    comm_size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

    extent =smpi_datatype_get_extent(datatype);
    smpi_datatype_extent(datatype, &true_lb, &true_extent);
    
    if (smpi_op_is_commute(op)) {
        is_commutative = 1;
    }

    disps = (int*)xbt_malloc( comm_size * sizeof(int));

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }
    
    if (total_count == 0) {
        xbt_free(disps);
        return MPI_ERR_COUNT;
    }

        if (sendbuf != MPI_IN_PLACE) {
            /* copy local data into recvbuf */
            smpi_datatype_copy(((char *)sendbuf+disps[rank]*extent),
                                       recvcounts[rank], datatype, recvbuf,
                                       recvcounts[rank], datatype);
        }
        
        /* allocate temporary buffer to store incoming data */
        tmp_recvbuf = (void*)smpi_get_tmp_recvbuffer(recvcounts[rank]*(MAX(true_extent,extent))+1);
        /* adjust for potential negative lower bound in datatype */
        tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);
        
        for (i=1; i<comm_size; i++) {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
            
            /* send the data that dst needs. recv data that this process
               needs from src into tmp_recvbuf */
            if (sendbuf != MPI_IN_PLACE) 
                smpi_mpi_sendrecv(((char *)sendbuf+disps[dst]*extent), 
                                             recvcounts[dst], datatype, dst,
                                             COLL_TAG_SCATTER, tmp_recvbuf,
                                             recvcounts[rank], datatype, src,
                                             COLL_TAG_SCATTER, comm,
                                             MPI_STATUS_IGNORE);
            else
                smpi_mpi_sendrecv(((char *)recvbuf+disps[dst]*extent), 
                                             recvcounts[dst], datatype, dst,
                                             COLL_TAG_SCATTER, tmp_recvbuf,
                                             recvcounts[rank], datatype, src,
                                             COLL_TAG_SCATTER, comm,
                                             MPI_STATUS_IGNORE);
            
            if (is_commutative || (src < rank)) {
                if (sendbuf != MPI_IN_PLACE) {
		     smpi_op_apply( op,
			                          tmp_recvbuf, recvbuf, &recvcounts[rank],
                               &datatype); 
                }
                else {
		    smpi_op_apply(op, 
			tmp_recvbuf, ((char *)recvbuf+disps[rank]*extent), 
			&recvcounts[rank], &datatype);
                    /* we can't store the result at the beginning of
                       recvbuf right here because there is useful data
                       there that other process/processes need. at the
                       end, we will copy back the result to the
                       beginning of recvbuf. */
                }
            }
            else {
                if (sendbuf != MPI_IN_PLACE) {
		    smpi_op_apply(op, 
		       recvbuf, tmp_recvbuf, &recvcounts[rank], &datatype);
                    /* copy result back into recvbuf */
                    mpi_errno = smpi_datatype_copy(tmp_recvbuf, recvcounts[rank],
                                               datatype, recvbuf,
                                               recvcounts[rank], datatype);
                    if (mpi_errno) return(mpi_errno);
                }
                else {
		    smpi_op_apply(op, 
                        ((char *)recvbuf+disps[rank]*extent),
			tmp_recvbuf, &recvcounts[rank], &datatype);
                    /* copy result back into recvbuf */
                    mpi_errno = smpi_datatype_copy(tmp_recvbuf, recvcounts[rank],
                                               datatype, 
                                               ((char *)recvbuf +
                                                disps[rank]*extent), 
                                               recvcounts[rank], datatype);
                    if (mpi_errno) return(mpi_errno);
                }
            }
        }
        
        /* if MPI_IN_PLACE, move output data to the beginning of
           recvbuf. already done for rank 0. */
        if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
            mpi_errno = smpi_datatype_copy(((char *)recvbuf +
                                        disps[rank]*extent),  
                                       recvcounts[rank], datatype,
                                       recvbuf, 
                                       recvcounts[rank], datatype );
            if (mpi_errno) return(mpi_errno);
        }
    
        xbt_free(disps);
        smpi_free_tmp_buffer(tmp_recvbuf);

        return MPI_SUCCESS;
}
    

int smpi_coll_tuned_reduce_scatter_mpich_noncomm(void *sendbuf, void *recvbuf, int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size = smpi_comm_size(comm) ;
    int rank = smpi_comm_rank(comm);
    int pof2;
    int log2_comm_size;
    int i, k;
    int recv_offset, send_offset;
    int block_size, total_count, size;
    MPI_Aint true_extent, true_lb;
    int buf0_was_inout;
    void *tmp_buf0;
    void *tmp_buf1;
    void *result_ptr;

    smpi_datatype_extent(datatype, &true_lb, &true_extent);

    pof2 = 1;
    log2_comm_size = 0;
    while (pof2 < comm_size) {
        pof2 <<= 1;
        ++log2_comm_size;
    }

    /* begin error checking */
    xbt_assert(pof2 == comm_size); /* FIXME this version only works for power of 2 procs */

    for (i = 0; i < (comm_size - 1); ++i) {
        xbt_assert(recvcounts[i] == recvcounts[i+1]);
    }
    /* end error checking */

    /* size of a block (count of datatype per block, NOT bytes per block) */
    block_size = recvcounts[0];
    total_count = block_size * comm_size;

    tmp_buf0=( void *)smpi_get_tmp_sendbuffer( true_extent * total_count);
    tmp_buf1=( void *)smpi_get_tmp_recvbuffer( true_extent * total_count);
    void *tmp_buf0_save=tmp_buf0;
    void *tmp_buf1_save=tmp_buf1;

    /* adjust for potential negative lower bound in datatype */
    tmp_buf0 = (void *)((char*)tmp_buf0 - true_lb);
    tmp_buf1 = (void *)((char*)tmp_buf1 - true_lb);

    /* Copy our send data to tmp_buf0.  We do this one block at a time and
       permute the blocks as we go according to the mirror permutation. */
    for (i = 0; i < comm_size; ++i) {
        mpi_errno = smpi_datatype_copy((char *)(sendbuf == MPI_IN_PLACE ? recvbuf : sendbuf) + (i * true_extent * block_size), block_size, datatype,
                                   (char *)tmp_buf0 + (MPIU_Mirror_permutation(i, log2_comm_size) * true_extent * block_size), block_size, datatype);
        if (mpi_errno) return(mpi_errno);
    }
    buf0_was_inout = 1;

    send_offset = 0;
    recv_offset = 0;
    size = total_count;
    for (k = 0; k < log2_comm_size; ++k) {
        /* use a double-buffering scheme to avoid local copies */
        char *incoming_data = (buf0_was_inout ? tmp_buf1 : tmp_buf0);
        char *outgoing_data = (buf0_was_inout ? tmp_buf0 : tmp_buf1);
        int peer = rank ^ (0x1 << k);
        size /= 2;

        if (rank > peer) {
            /* we have the higher rank: send top half, recv bottom half */
            recv_offset += size;
        }
        else {
            /* we have the lower rank: recv top half, send bottom half */
            send_offset += size;
        }

        smpi_mpi_sendrecv(outgoing_data + send_offset*true_extent,
                                     size, datatype, peer, COLL_TAG_SCATTER,
                                     incoming_data + recv_offset*true_extent,
                                     size, datatype, peer, COLL_TAG_SCATTER,
                                     comm, MPI_STATUS_IGNORE);
        /* always perform the reduction at recv_offset, the data at send_offset
           is now our peer's responsibility */
        if (rank > peer) {
            /* higher ranked value so need to call op(received_data, my_data) */
            smpi_op_apply(op, 
                   incoming_data + recv_offset*true_extent,
                     outgoing_data + recv_offset*true_extent,
                     &size, &datatype );
            /* buf0_was_inout = buf0_was_inout; */
        }
        else {
            /* lower ranked value so need to call op(my_data, received_data) */
	    smpi_op_apply( op,
		     outgoing_data + recv_offset*true_extent,
                     incoming_data + recv_offset*true_extent,
                     &size, &datatype);
            buf0_was_inout = !buf0_was_inout;
        }

        /* the next round of send/recv needs to happen within the block (of size
           "size") that we just received and reduced */
        send_offset = recv_offset;
    }

    xbt_assert(size == recvcounts[rank]);

    /* copy the reduced data to the recvbuf */
    result_ptr = (char *)(buf0_was_inout ? tmp_buf0 : tmp_buf1) + recv_offset * true_extent;
    mpi_errno = smpi_datatype_copy(result_ptr, size, datatype,
                               recvbuf, size, datatype);
    smpi_free_tmp_buffer(tmp_buf0_save);
    smpi_free_tmp_buffer(tmp_buf1_save);
    if (mpi_errno) return(mpi_errno);
    return MPI_SUCCESS;
}



int smpi_coll_tuned_reduce_scatter_mpich_rdb(void *sendbuf, void *recvbuf, int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb; 
    int  *disps;
    void *tmp_recvbuf, *tmp_results;
    int mpi_errno = MPI_SUCCESS;
    int dis[2], blklens[2], total_count, dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    int received;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative=0;
    comm_size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

    extent =smpi_datatype_get_extent(datatype);
    smpi_datatype_extent(datatype, &true_lb, &true_extent);
    
    if (smpi_op_is_commute(op)) {
        is_commutative = 1;
    }

    disps = (int*)xbt_malloc( comm_size * sizeof(int));

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }
    
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */

            /* need to allocate temporary buffer to receive incoming data*/
            tmp_recvbuf= (void *) smpi_get_tmp_recvbuffer( total_count*(MAX(true_extent,extent)));
            /* adjust for potential negative lower bound in datatype */
            tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

            /* need to allocate another temporary buffer to accumulate
               results */
            tmp_results = (void *)smpi_get_tmp_sendbuffer( total_count*(MAX(true_extent,extent)));
            /* adjust for potential negative lower bound in datatype */
            tmp_results = (void *)((char*)tmp_results - true_lb);

            /* copy sendbuf into tmp_results */
            if (sendbuf != MPI_IN_PLACE)
                mpi_errno = smpi_datatype_copy(sendbuf, total_count, datatype,
                                           tmp_results, total_count, datatype);
            else
                mpi_errno = smpi_datatype_copy(recvbuf, total_count, datatype,
                                           tmp_results, total_count, datatype);

            if (mpi_errno) return(mpi_errno);

            mask = 0x1;
            i = 0;
            while (mask < comm_size) {
                dst = rank ^ mask;

                dst_tree_root = dst >> i;
                dst_tree_root <<= i;

                my_tree_root = rank >> i;
                my_tree_root <<= i;

                /* At step 1, processes exchange (n-n/p) amount of
                   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
                   amount of data, and so forth. We use derived datatypes for this.

                   At each step, a process does not need to send data
                   indexed from my_tree_root to
                   my_tree_root+mask-1. Similarly, a process won't receive
                   data indexed from dst_tree_root to dst_tree_root+mask-1. */

                /* calculate sendtype */
                blklens[0] = blklens[1] = 0;
                for (j=0; j<my_tree_root; j++)
                    blklens[0] += recvcounts[j];
                for (j=my_tree_root+mask; j<comm_size; j++)
                    blklens[1] += recvcounts[j];

                dis[0] = 0;
                dis[1] = blklens[0];
                for (j=my_tree_root; (j<my_tree_root+mask) && (j<comm_size); j++)
                    dis[1] += recvcounts[j];

                mpi_errno = smpi_datatype_indexed(2, blklens, dis, datatype, &sendtype);
                if (mpi_errno) return(mpi_errno);
                
                smpi_datatype_commit(&sendtype);

                /* calculate recvtype */
                blklens[0] = blklens[1] = 0;
                for (j=0; j<dst_tree_root && j<comm_size; j++)
                    blklens[0] += recvcounts[j];
                for (j=dst_tree_root+mask; j<comm_size; j++)
                    blklens[1] += recvcounts[j];

                dis[0] = 0;
                dis[1] = blklens[0];
                for (j=dst_tree_root; (j<dst_tree_root+mask) && (j<comm_size); j++)
                    dis[1] += recvcounts[j];

                mpi_errno = smpi_datatype_indexed(2, blklens, dis, datatype, &recvtype);
                if (mpi_errno) return(mpi_errno);
                
                smpi_datatype_commit(&recvtype);

                received = 0;
                if (dst < comm_size) {
                    /* tmp_results contains data to be sent in each step. Data is
                       received in tmp_recvbuf and then accumulated into
                       tmp_results. accumulation is done later below.   */ 

                    smpi_mpi_sendrecv(tmp_results, 1, sendtype, dst,
                                                 COLL_TAG_SCATTER,
                                                 tmp_recvbuf, 1, recvtype, dst,
                                                 COLL_TAG_SCATTER, comm,
                                                 MPI_STATUS_IGNORE);
                    received = 1;
                }

                /* if some processes in this process's subtree in this step
                   did not have any destination process to communicate with
                   because of non-power-of-two, we need to send them the
                   result. We use a logarithmic recursive-halfing algorithm
                   for this. */

                if (dst_tree_root + mask > comm_size) {
                    nprocs_completed = comm_size - my_tree_root - mask;
                    /* nprocs_completed is the number of processes in this
                       subtree that have all the data. Send data to others
                       in a tree fashion. First find root of current tree
                       that is being divided into two. k is the number of
                       least-significant bits in this process's rank that
                       must be zeroed out to find the rank of the root */ 
                    j = mask;
                    k = 0;
                    while (j) {
                        j >>= 1;
                        k++;
                    }
                    k--;

                    tmp_mask = mask >> 1;
                    while (tmp_mask) {
                        dst = rank ^ tmp_mask;

                        tree_root = rank >> k;
                        tree_root <<= k;

                        /* send only if this proc has data and destination
                           doesn't have data. at any step, multiple processes
                           can send if they have the data */
                        if ((dst > rank) && 
                            (rank < tree_root + nprocs_completed)
                            && (dst >= tree_root + nprocs_completed)) {
                            /* send the current result */
                            smpi_mpi_send(tmp_recvbuf, 1, recvtype,
                                                     dst, COLL_TAG_SCATTER,
                                                     comm);
                        }
                        /* recv only if this proc. doesn't have data and sender
                           has data */
                        else if ((dst < rank) && 
                                 (dst < tree_root + nprocs_completed) &&
                                 (rank >= tree_root + nprocs_completed)) {
                            smpi_mpi_recv(tmp_recvbuf, 1, recvtype, dst,
                                                     COLL_TAG_SCATTER,
                                                     comm, MPI_STATUS_IGNORE); 
                            received = 1;
                        }
                        tmp_mask >>= 1;
                        k--;
                    }
                }

                /* The following reduction is done here instead of after 
                   the MPIC_Sendrecv_ft or MPIC_Recv_ft above. This is
                   because to do it above, in the noncommutative 
                   case, we would need an extra temp buffer so as not to
                   overwrite temp_recvbuf, because temp_recvbuf may have
                   to be communicated to other processes in the
                   non-power-of-two case. To avoid that extra allocation,
                   we do the reduce here. */
                if (received) {
                    if (is_commutative || (dst_tree_root < my_tree_root)) {
                        {
			         smpi_op_apply(op, 
                               tmp_recvbuf, tmp_results, &blklens[0],
			       &datatype); 
			        smpi_op_apply(op, 
                               ((char *)tmp_recvbuf + dis[1]*extent),
			       ((char *)tmp_results + dis[1]*extent),
			       &blklens[1], &datatype); 
                        }
                    }
                    else {
                        {
			         smpi_op_apply(op,
                                   tmp_results, tmp_recvbuf, &blklens[0],
                                   &datatype); 
			         smpi_op_apply(op,
                                   ((char *)tmp_results + dis[1]*extent),
                                   ((char *)tmp_recvbuf + dis[1]*extent),
                                   &blklens[1], &datatype); 
                        }
                        /* copy result back into tmp_results */
                        mpi_errno = smpi_datatype_copy(tmp_recvbuf, 1, recvtype, 
                                                   tmp_results, 1, recvtype);
                        if (mpi_errno) return(mpi_errno);
                    }
                }

                smpi_datatype_free(&sendtype);
                smpi_datatype_free(&recvtype);

                mask <<= 1;
                i++;
            }

            /* now copy final results from tmp_results to recvbuf */
            mpi_errno = smpi_datatype_copy(((char *)tmp_results+disps[rank]*extent),
                                       recvcounts[rank], datatype, recvbuf,
                                       recvcounts[rank], datatype);
            if (mpi_errno) return(mpi_errno);

    xbt_free(disps);
    smpi_free_tmp_buffer(tmp_recvbuf);
    smpi_free_tmp_buffer(tmp_results);
    return MPI_SUCCESS;
        }


