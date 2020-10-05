/* selector for collective algorithms based on openmpi's default coll_tuned_decision_fixed selector */

/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"

#include <memory>

namespace simgrid {
namespace smpi {

int allreduce__ompi(const void *sbuf, void *rbuf, int count,
                    MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
    size_t dsize, block_dsize;
    int comm_size = comm->size();
    const size_t intermediate_message = 10000;

    /**
     * Decision function based on MX results from the Grig cluster at UTK.
     *
     * Currently, linear, recursive doubling, and nonoverlapping algorithms
     * can handle both commutative and non-commutative operations.
     * Ring algorithm does not support non-commutative operations.
     */
    dsize = dtype->size();
    block_dsize = dsize * count;

    if (block_dsize < intermediate_message) {
        return allreduce__rdb(sbuf, rbuf, count, dtype, op, comm);
    }

    if( ((op==MPI_OP_NULL) || op->is_commutative()) && (count > comm_size) ) {
        const size_t segment_size = 1 << 20; /* 1 MB */
        if ((comm_size * segment_size >= block_dsize)) {
            //FIXME: ok, these are not the right algorithms, try to find closer ones
            // lr is a good match for allreduce_ring (difference is mainly the use of sendrecv)
            return allreduce__lr(sbuf, rbuf, count, dtype, op, comm);
        } else {
           return allreduce__ompi_ring_segmented(sbuf, rbuf, count, dtype, op, comm /*segment_size*/);
        }
    }

    return allreduce__redbcast(sbuf, rbuf, count, dtype, op, comm);
}



int alltoall__ompi(const void *sbuf, int scount,
                   MPI_Datatype sdtype,
                   void* rbuf, int rcount,
                   MPI_Datatype rdtype,
                   MPI_Comm comm)
{
    int communicator_size;
    size_t dsize, block_dsize;
    communicator_size = comm->size();

    /* Decision function based on measurement on Grig cluster at
       the University of Tennessee (2GB MX) up to 64 nodes.
       Has better performance for messages of intermediate sizes than the old one */
    /* determine block size */
    dsize = sdtype->size();
    block_dsize = dsize * scount;

    if ((block_dsize < 200) && (communicator_size > 12)) {
        return alltoall__bruck(sbuf, scount, sdtype,
                               rbuf, rcount, rdtype, comm);

    } else if (block_dsize < 3000) {
        return alltoall__basic_linear(sbuf, scount, sdtype,
                                      rbuf, rcount, rdtype, comm);
    }

    return alltoall__ring(sbuf, scount, sdtype,
                          rbuf, rcount, rdtype, comm);
}

int alltoallv__ompi(const void *sbuf, const int *scounts, const int *sdisps,
                    MPI_Datatype sdtype,
                    void *rbuf, const int *rcounts, const int *rdisps,
                    MPI_Datatype rdtype,
                    MPI_Comm  comm
                    )
{
    /* For starters, just keep the original algorithm. */
    return alltoallv__ring(sbuf, scounts, sdisps, sdtype,
                           rbuf, rcounts, rdisps,rdtype,
                           comm);
}

int barrier__ompi(MPI_Comm  comm)
{    int communicator_size = comm->size();

    if( 2 == communicator_size )
        return barrier__ompi_two_procs(comm);
/*     * Basic optimisation. If we have a power of 2 number of nodes*/
/*     * the use the recursive doubling algorithm, otherwise*/
/*     * bruck is the one we want.*/
    {
        bool has_one = false;
        for( ; communicator_size > 0; communicator_size >>= 1 ) {
            if( communicator_size & 0x1 ) {
                if( has_one )
                    return barrier__ompi_bruck(comm);
                has_one = true;
            }
        }
    }
    return barrier__ompi_recursivedoubling(comm);
}

int bcast__ompi(void *buff, int count, MPI_Datatype datatype, int root, MPI_Comm  comm)
{
    /* Decision function based on MX results for
       messages up to 36MB and communicator sizes up to 64 nodes */
    const size_t small_message_size = 2048;
    const size_t intermediate_message_size = 370728;
    const double a_p16  = 3.2118e-6; /* [1 / byte] */
    const double b_p16  = 8.7936;
    const double a_p64  = 2.3679e-6; /* [1 / byte] */
    const double b_p64  = 1.1787;
    const double a_p128 = 1.6134e-6; /* [1 / byte] */
    const double b_p128 = 2.1102;

    int communicator_size;
    //int segsize = 0;
    size_t message_size, dsize;

    communicator_size = comm->size();

    /* else we need data size for decision function */
    dsize = datatype->size();
    message_size = dsize * (unsigned long)count;   /* needed for decision */

    /* Handle messages of small and intermediate size, and
       single-element broadcasts */
    if ((message_size < small_message_size) || (count <= 1)) {
        /* Binomial without segmentation */
        return bcast__binomial_tree(buff, count, datatype, root, comm);

    } else if (message_size < intermediate_message_size) {
        // SplittedBinary with 1KB segments
        return bcast__ompi_split_bintree(buff, count, datatype, root, comm);

    }
     //Handle large message sizes
    else if (communicator_size < (a_p128 * message_size + b_p128)) {
        //Pipeline with 128KB segments
        //segsize = 1024  << 7;
        return bcast__ompi_pipeline(buff, count, datatype, root, comm);


    } else if (communicator_size < 13) {
        // Split Binary with 8KB segments
        return bcast__ompi_split_bintree(buff, count, datatype, root, comm);

    } else if (communicator_size < (a_p64 * message_size + b_p64)) {
        // Pipeline with 64KB segments
        //segsize = 1024 << 6;
        return bcast__ompi_pipeline(buff, count, datatype, root, comm);


    } else if (communicator_size < (a_p16 * message_size + b_p16)) {
        //Pipeline with 16KB segments
        //segsize = 1024 << 4;
        return bcast__ompi_pipeline(buff, count, datatype, root, comm);


    }
    /* Pipeline with 8KB segments */
    //segsize = 1024 << 3;
    return bcast__flattree_pipeline(buff, count, datatype, root, comm /*segsize*/);
#if 0
    /* this is based on gige measurements */

    if (communicator_size  < 4) {
        return bcast__intra_basic_linear(buff, count, datatype, root, comm, module);
    }
    if (communicator_size == 4) {
        if (message_size < 524288) segsize = 0;
        else segsize = 16384;
        return bcast__intra_bintree(buff, count, datatype, root, comm, module, segsize);
    }
    if (communicator_size <= 8 && message_size < 4096) {
        return bcast__intra_basic_linear(buff, count, datatype, root, comm, module);
    }
    if (communicator_size > 8 && message_size >= 32768 && message_size < 524288) {
        segsize = 16384;
        return  bcast__intra_bintree(buff, count, datatype, root, comm, module, segsize);
    }
    if (message_size >= 524288) {
        segsize = 16384;
        return bcast__intra_pipeline(buff, count, datatype, root, comm, module, segsize);
    }
    segsize = 0;
    /* once tested can swap this back in */
    /* return bcast__intra_bmtree(buff, count, datatype, root, comm, segsize); */
    return bcast__intra_bintree(buff, count, datatype, root, comm, module, segsize);
#endif  /* 0 */
}

int reduce__ompi(const void *sendbuf, void *recvbuf,
                 int count, MPI_Datatype  datatype,
                 MPI_Op   op, int root,
                 MPI_Comm   comm)
{
    int communicator_size=0;
    //int segsize = 0;
    size_t message_size, dsize;
    const double a1 =  0.6016 / 1024.0; /* [1/B] */
    const double b1 =  1.3496;
    const double a2 =  0.0410 / 1024.0; /* [1/B] */
    const double b2 =  9.7128;
    const double a3 =  0.0422 / 1024.0; /* [1/B] */
    const double b3 =  1.1614;
    //const double a4 =  0.0033 / 1024.0;  [1/B]
    //const double b4 =  1.6761;

    /* no limit on # of outstanding requests */
    //const int max_requests = 0;

    communicator_size = comm->size();

    /* need data size for decision function */
    dsize=datatype->size();
    message_size = dsize * count;   /* needed for decision */

    /**
     * If the operation is non commutative we currently have choice of linear
     * or in-order binary tree algorithm.
     */
    if ((op != MPI_OP_NULL) && not op->is_commutative()) {
      if ((communicator_size < 12) && (message_size < 2048)) {
        return reduce__ompi_basic_linear(sendbuf, recvbuf, count, datatype, op, root, comm /*, module*/);
      }
      return reduce__ompi_in_order_binary(sendbuf, recvbuf, count, datatype, op, root, comm /*, module,
                                                             0, max_requests*/);
    }

    if ((communicator_size < 8) && (message_size < 512)){
        /* Linear_0K */
        return reduce__ompi_basic_linear(sendbuf, recvbuf, count, datatype, op, root, comm);
    } else if (((communicator_size < 8) && (message_size < 20480)) ||
               (message_size < 2048) || (count <= 1)) {
        /* Binomial_0K */
        //segsize = 0;
        return reduce__ompi_binomial(sendbuf, recvbuf, count, datatype, op, root, comm/*, module, segsize, max_requests*/);
    } else if (communicator_size > (a1 * message_size + b1)) {
        // Binomial_1K
        //segsize = 1024;
        return reduce__ompi_binomial(sendbuf, recvbuf, count, datatype, op, root, comm/*, module,
                                                     segsize, max_requests*/);
    } else if (communicator_size > (a2 * message_size + b2)) {
        // Pipeline_1K
        //segsize = 1024;
        return reduce__ompi_pipeline(sendbuf, recvbuf, count, datatype, op, root, comm/*, module,
                                                      segsize, max_requests*/);
    } else if (communicator_size > (a3 * message_size + b3)) {
        // Binary_32K
        //segsize = 32*1024;
        return reduce__ompi_binary( sendbuf, recvbuf, count, datatype, op, root,
                                                    comm/*, module, segsize, max_requests*/);
    }
//    if (communicator_size > (a4 * message_size + b4)) {
        // Pipeline_32K
//        segsize = 32*1024;
//    } else {
        // Pipeline_64K
//        segsize = 64*1024;
//    }
    return reduce__ompi_pipeline(sendbuf, recvbuf, count, datatype, op, root, comm/*, module,
                                                  segsize, max_requests*/);

#if 0
    /* for small messages use linear algorithm */
    if (message_size <= 4096) {
        segsize = 0;
        fanout = communicator_size - 1;
        /* when linear implemented or taken from basic put here, right now using chain as a linear system */
        /* it is implemented and I shouldn't be calling a chain with a fanout bigger than MAXTREEFANOUT from topo.h! */
        return reduce__intra_basic_linear(sendbuf, recvbuf, count, datatype, op, root, comm, module);
        /*        return reduce__intra_chain(sendbuf, recvbuf, count, datatype, op, root, comm, segsize, fanout); */
    }
    if (message_size < 524288) {
        if (message_size <= 65536 ) {
            segsize = 32768;
            fanout = 8;
        } else {
            segsize = 1024;
            fanout = communicator_size/2;
        }
        /* later swap this for a binary tree */
        /*         fanout = 2; */
        return reduce__intra_chain(sendbuf, recvbuf, count, datatype, op, root, comm, module,
                                   segsize, fanout, max_requests);
    }
    segsize = 1024;
    return reduce__intra_pipeline(sendbuf, recvbuf, count, datatype, op, root, comm, module,
                                  segsize, max_requests);
#endif  /* 0 */
}

int reduce_scatter__ompi(const void *sbuf, void *rbuf,
                         const int *rcounts,
                         MPI_Datatype dtype,
                         MPI_Op  op,
                         MPI_Comm  comm
                         )
{
    int comm_size, i, pow2;
    size_t total_message_size, dsize;
    const double a = 0.0012;
    const double b = 8.0;
    const size_t small_message_size = 12 * 1024;
    const size_t large_message_size = 256 * 1024;
    int zerocounts = 0;

    XBT_DEBUG("reduce_scatter__ompi");

    comm_size = comm->size();
    // We need data size for decision function
    dsize=dtype->size();
    total_message_size = 0;
    for (i = 0; i < comm_size; i++) {
        total_message_size += rcounts[i];
        if (0 == rcounts[i]) {
            zerocounts = 1;
        }
    }

    if (((op != MPI_OP_NULL) && not op->is_commutative()) || (zerocounts)) {
      reduce_scatter__default(sbuf, rbuf, rcounts, dtype, op, comm);
      return MPI_SUCCESS;
    }

    total_message_size *= dsize;

    // compute the nearest power of 2
    for (pow2 = 1; pow2 < comm_size; pow2 <<= 1);

    if ((total_message_size <= small_message_size) ||
        ((total_message_size <= large_message_size) && (pow2 == comm_size)) ||
        (comm_size >= a * total_message_size + b)) {
        return reduce_scatter__ompi_basic_recursivehalving(sbuf, rbuf, rcounts, dtype, op, comm);
    }
    return reduce_scatter__ompi_ring(sbuf, rbuf, rcounts, dtype, op, comm);
}

int allgather__ompi(const void *sbuf, int scount,
                    MPI_Datatype sdtype,
                    void* rbuf, int rcount,
                    MPI_Datatype rdtype,
                    MPI_Comm  comm
                    )
{
    int communicator_size, pow2_size;
    size_t dsize, total_dsize;

    communicator_size = comm->size();

    /* Special case for 2 processes */
    if (communicator_size == 2) {
        return allgather__pair(sbuf, scount, sdtype,
                               rbuf, rcount, rdtype,
                               comm/*, module*/);
    }

    /* Determine complete data size */
    dsize=sdtype->size();
    total_dsize = dsize * scount * communicator_size;

    for (pow2_size  = 1; pow2_size < communicator_size; pow2_size <<=1);

    /* Decision based on MX 2Gb results from Grig cluster at
       The University of Tennesse, Knoxville
       - if total message size is less than 50KB use either bruck or
       recursive doubling for non-power of two and power of two nodes,
       respectively.
       - else use ring and neighbor exchange algorithms for odd and even
       number of nodes, respectively.
    */
    if (total_dsize < 50000) {
        if (pow2_size == communicator_size) {
            return allgather__rdb(sbuf, scount, sdtype,
                                  rbuf, rcount, rdtype,
                                  comm);
        } else {
            return allgather__bruck(sbuf, scount, sdtype,
                                    rbuf, rcount, rdtype,
                                    comm);
        }
    } else {
        if (communicator_size % 2) {
            return allgather__ring(sbuf, scount, sdtype,
                                   rbuf, rcount, rdtype,
                                   comm);
        } else {
            return allgather__ompi_neighborexchange(sbuf, scount, sdtype,
                                                    rbuf, rcount, rdtype,
                                                    comm);
        }
    }

#if defined(USE_MPICH2_DECISION)
    /* Decision as in MPICH-2
       presented in Thakur et.al. "Optimization of Collective Communication
       Operations in MPICH", International Journal of High Performance Computing
       Applications, Vol. 19, No. 1, 49-66 (2005)
       - for power-of-two processes and small and medium size messages
       (up to 512KB) use recursive doubling
       - for non-power-of-two processes and small messages (80KB) use bruck,
       - for everything else use ring.
    */
    if ((pow2_size == communicator_size) && (total_dsize < 524288)) {
        return allgather__rdb(sbuf, scount, sdtype,
                              rbuf, rcount, rdtype,
                              comm);
    } else if (total_dsize <= 81920) {
        return allgather__bruck(sbuf, scount, sdtype,
                                rbuf, rcount, rdtype,
                                comm);
    }
    return allgather__ring(sbuf, scount, sdtype,
                           rbuf, rcount, rdtype,
                           comm);
#endif  /* defined(USE_MPICH2_DECISION) */
}

int allgatherv__ompi(const void *sbuf, int scount,
                     MPI_Datatype sdtype,
                     void* rbuf, const int *rcounts,
                     const int *rdispls,
                     MPI_Datatype rdtype,
                     MPI_Comm  comm
                     )
{
    int i;
    int communicator_size;
    size_t dsize, total_dsize;

    communicator_size = comm->size();

    /* Special case for 2 processes */
    if (communicator_size == 2) {
        return allgatherv__pair(sbuf, scount, sdtype,
                                rbuf, rcounts, rdispls, rdtype,
                                comm);
    }

    /* Determine complete data size */
    dsize=sdtype->size();
    total_dsize = 0;
    for (i = 0; i < communicator_size; i++) {
        total_dsize += dsize * rcounts[i];
    }

    /* Decision based on allgather decision.   */
    if (total_dsize < 50000) {
        return allgatherv__ompi_bruck(sbuf, scount, sdtype,
                                      rbuf, rcounts, rdispls, rdtype,
                                      comm);

    } else {
        if (communicator_size % 2) {
            return allgatherv__ring(sbuf, scount, sdtype,
                                    rbuf, rcounts, rdispls, rdtype,
                                    comm);
        } else {
            return  allgatherv__ompi_neighborexchange(sbuf, scount, sdtype,
                                                      rbuf, rcounts, rdispls, rdtype,
                                                      comm);
        }
    }
}

int gather__ompi(const void *sbuf, int scount,
                 MPI_Datatype sdtype,
                 void* rbuf, int rcount,
                 MPI_Datatype rdtype,
                 int root,
                 MPI_Comm  comm
                 )
{
    //const int large_segment_size = 32768;
    //const int small_segment_size = 1024;

    //const size_t large_block_size = 92160;
    const size_t intermediate_block_size = 6000;
    const size_t small_block_size = 1024;

    const int large_communicator_size = 60;
    const int small_communicator_size = 10;

    int communicator_size, rank;
    size_t dsize, block_size;

    XBT_DEBUG("smpi_coll_tuned_gather_ompi");

    communicator_size = comm->size();
    rank = comm->rank();

    // Determine block size
    if (rank == root) {
        dsize = rdtype->size();
        block_size = dsize * rcount;
    } else {
        dsize = sdtype->size();
        block_size = dsize * scount;
    }

/*    if (block_size > large_block_size) {*/
/*        return smpi_coll_tuned_gather_ompi_linear_sync (sbuf, scount, sdtype, */
/*                                                         rbuf, rcount, rdtype, */
/*                                                         root, comm);*/

/*    } else*/ if (block_size > intermediate_block_size) {
        return gather__ompi_linear_sync(sbuf, scount, sdtype,
                                        rbuf, rcount, rdtype,
                                        root, comm);

    } else if ((communicator_size > large_communicator_size) ||
               ((communicator_size > small_communicator_size) &&
                (block_size < small_block_size))) {
        return gather__ompi_binomial(sbuf, scount, sdtype,
                                     rbuf, rcount, rdtype,
                                     root, comm);

    }
    // Otherwise, use basic linear
    return gather__ompi_basic_linear(sbuf, scount, sdtype,
                                     rbuf, rcount, rdtype,
                                     root, comm);
}


int scatter__ompi(const void *sbuf, int scount,
                  MPI_Datatype sdtype,
                  void* rbuf, int rcount,
                  MPI_Datatype rdtype,
                  int root, MPI_Comm  comm
                  )
{
    const size_t small_block_size = 300;
    const int small_comm_size = 10;
    int communicator_size, rank;
    size_t dsize, block_size;

    XBT_DEBUG("Coll_scatter_ompi::scatter");

    communicator_size = comm->size();
    rank = comm->rank();
    // Determine block size
    if (root == rank) {
        dsize=sdtype->size();
        block_size = dsize * scount;
    } else {
        dsize=rdtype->size();
        block_size = dsize * rcount;
    }

    if ((communicator_size > small_comm_size) &&
        (block_size < small_block_size)) {
      std::unique_ptr<unsigned char[]> tmp_buf;
      if (rank != root) {
        tmp_buf = std::make_unique<unsigned char[]>(rcount * rdtype->get_extent());
        sbuf   = tmp_buf.get();
        scount = rcount;
        sdtype = rdtype;
      }
      return scatter__ompi_binomial(sbuf, scount, sdtype, rbuf, rcount, rdtype, root, comm);
    }
    return scatter__ompi_basic_linear(sbuf, scount, sdtype,
                                      rbuf, rcount, rdtype,
                                      root, comm);
}

}
}
