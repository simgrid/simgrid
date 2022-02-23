/* selector for collective algorithms based on openmpi's default coll_tuned_decision_fixed selector
 * Updated 02/2022                                                          */

/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"

#include <memory>

/* FIXME
add algos:
allreduce nonoverlapping, basic linear
alltoall linear_sync
bcast chain
reduce_scatter butterfly
scatter linear_nb
*/

namespace simgrid {
namespace smpi {

int allreduce__ompi(const void *sbuf, void *rbuf, int count,
                    MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
    size_t total_dsize = dtype->size() * (ptrdiff_t)count;
    int communicator_size = comm->size();
    int alg = 1;
    int(*funcs[]) (const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm)={
        &allreduce__redbcast,
        &allreduce__redbcast,
        &allreduce__rdb,
        &allreduce__lr,
        &allreduce__ompi_ring_segmented,
        &allreduce__rab_rdb
    };

    /** Algorithms:
     *  {1, "basic_linear"},
     *  {2, "nonoverlapping"},
     *  {3, "recursive_doubling"},
     *  {4, "ring"},
     *  {5, "segmented_ring"},
     *  {6, "rabenseifner"
     *
     * Currently, ring, segmented ring, and rabenseifner do not support
     * non-commutative operations.
     */
    if ((op != MPI_OP_NULL) && not op->is_commutative()) {
        if (communicator_size < 4) {
            if (total_dsize < 131072) {
                alg = 3;
            } else {
                alg = 1;
            }
        } else if (communicator_size < 8) {
            alg = 3;
        } else if (communicator_size < 16) {
            if (total_dsize < 1048576) {
                alg = 3;
            } else {
                alg = 2;
            }
        } else if (communicator_size < 128) {
            alg = 3;
        } else if (communicator_size < 256) {
            if (total_dsize < 131072) {
                alg = 2;
            } else if (total_dsize < 524288) {
                alg = 3;
            } else {
                alg = 2;
            }
        } else if (communicator_size < 512) {
            if (total_dsize < 4096) {
                alg = 2;
            } else if (total_dsize < 524288) {
                alg = 3;
            } else {
                alg = 2;
            }
        } else {
            if (total_dsize < 2048) {
                alg = 2;
            } else {
                alg = 3;
            }
        }
    } else {
        if (communicator_size < 4) {
            if (total_dsize < 8) {
                alg = 4;
            } else if (total_dsize < 4096) {
                alg = 3;
            } else if (total_dsize < 8192) {
                alg = 4;
            } else if (total_dsize < 16384) {
                alg = 3;
            } else if (total_dsize < 65536) {
                alg = 4;
            } else if (total_dsize < 262144) {
                alg = 5;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 8) {
            if (total_dsize < 16) {
                alg = 4;
            } else if (total_dsize < 8192) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 16) {
            if (total_dsize < 8192) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 32) {
            if (total_dsize < 64) {
                alg = 5;
            } else if (total_dsize < 4096) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 64) {
            if (total_dsize < 128) {
                alg = 5;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 128) {
            if (total_dsize < 262144) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 256) {
            if (total_dsize < 131072) {
                alg = 2;
            } else if (total_dsize < 262144) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 512) {
            if (total_dsize < 4096) {
                alg = 2;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 2048) {
            if (total_dsize < 2048) {
                alg = 2;
            } else if (total_dsize < 16384) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 4096) {
            if (total_dsize < 2048) {
                alg = 2;
            } else if (total_dsize < 4096) {
                alg = 5;
            } else if (total_dsize < 16384) {
                alg = 3;
            } else {
                alg = 6;
            }
        } else {
            if (total_dsize < 2048) {
                alg = 2;
            } else if (total_dsize < 16384) {
                alg = 5;
            } else if (total_dsize < 32768) {
                alg = 3;
            } else {
                alg = 6;
            }
        }
    }
    return funcs[alg-1](sbuf, rbuf, count, dtype, op, comm);
}



int alltoall__ompi(const void *sbuf, int scount,
                   MPI_Datatype sdtype,
                   void* rbuf, int rcount,
                   MPI_Datatype rdtype,
                   MPI_Comm comm)
{
    int alg = 1;
    size_t dsize, total_dsize;
    int communicator_size = comm->size();

    if (MPI_IN_PLACE != sbuf) {
        dsize = sdtype->size();
    } else {
        dsize = rdtype->size();
    }
    total_dsize = dsize * (ptrdiff_t)scount;
    int (*funcs[])(const void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm) = {
        &alltoall__basic_linear,
        &alltoall__pair,
        &alltoall__bruck,
        &alltoall__basic_linear,
        &alltoall__basic_linear
    };
    /** Algorithms:
     *  {1, "linear"},
     *  {2, "pairwise"},
     *  {3, "modified_bruck"},
     *  {4, "linear_sync"},
     *  {5, "two_proc"},
     */
    if (communicator_size == 2) {
        if (total_dsize < 2) {
            alg = 2;
        } else if (total_dsize < 4) {
            alg = 5;
        } else if (total_dsize < 16) {
            alg = 2;
        } else if (total_dsize < 64) {
            alg = 5;
        } else if (total_dsize < 256) {
            alg = 2;
        } else if (total_dsize < 4096) {
            alg = 5;
        } else if (total_dsize < 32768) {
            alg = 2;
        } else if (total_dsize < 262144) {
            alg = 4;
        } else if (total_dsize < 1048576) {
            alg = 5;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 8) {
        if (total_dsize < 8192) {
            alg = 4;
        } else if (total_dsize < 16384) {
            alg = 1;
        } else if (total_dsize < 65536) {
            alg = 4;
        } else if (total_dsize < 524288) {
            alg = 1;
        } else if (total_dsize < 1048576) {
            alg = 2;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 16) {
        if (total_dsize < 262144) {
            alg = 4;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 32) {
        if (total_dsize < 4) {
            alg = 4;
        } else if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 8192) {
            alg = 4;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else if (total_dsize < 262144) {
            alg = 4;
        } else if (total_dsize < 524288) {
            alg = 1;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 64) {
        if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 524288) {
            alg = 1;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 128) {
        if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 2048) {
            alg = 1;
        } else if (total_dsize < 4096) {
            alg = 4;
        } else if (total_dsize < 262144) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 256) {
        if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 2048) {
            alg = 4;
        } else if (total_dsize < 262144) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 512) {
        if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 8192) {
            alg = 4;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 1024) {
        if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 8192) {
            alg = 4;
        } else if (total_dsize < 16384) {
            alg = 1;
        } else if (total_dsize < 131072) {
            alg = 4;
        } else if (total_dsize < 262144) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 2048) {
        if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 1024) {
            alg = 4;
        } else if (total_dsize < 2048) {
            alg = 1;
        } else if (total_dsize < 16384) {
            alg = 4;
        } else if (total_dsize < 262144) {
            alg = 1;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 4096) {
        if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 4096) {
            alg = 4;
        } else if (total_dsize < 8192) {
            alg = 1;
        } else if (total_dsize < 131072) {
            alg = 4;
        } else {
            alg = 1;
        }
    } else {
        if (total_dsize < 2048) {
            alg = 3;
        } else if (total_dsize < 8192) {
            alg = 4;
        } else if (total_dsize < 16384) {
            alg = 1;
        } else if (total_dsize < 32768) {
            alg = 4;
        } else if (total_dsize < 65536) {
            alg = 1;
        } else {
            alg = 4;
        }
    }

    return funcs[alg-1](sbuf, scount, sdtype,
                          rbuf, rcount, rdtype, comm);
}

int alltoallv__ompi(const void *sbuf, const int *scounts, const int *sdisps,
                    MPI_Datatype sdtype,
                    void *rbuf, const int *rcounts, const int *rdisps,
                    MPI_Datatype rdtype,
                    MPI_Comm  comm
                    )
{
    int communicator_size = comm->size();
    int alg = 1;
    int (*funcs[])(const void *, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm) = {
        &alltoallv__ompi_basic_linear,
        &alltoallv__pair
    };
   /** Algorithms:
     *  {1, "basic_linear"},
     *  {2, "pairwise"},
     *
     * We can only optimize based on com size
     */
    if (communicator_size < 4) {
        alg = 2;
    } else if (communicator_size < 64) {
        alg = 1;
    } else if (communicator_size < 128) {
        alg = 2;
    } else if (communicator_size < 256) {
        alg = 1;
    } else if (communicator_size < 1024) {
        alg = 2;
    } else {
        alg = 1;
    }
    return funcs[alg-1](sbuf, scounts, sdisps, sdtype,
                           rbuf, rcounts, rdisps,rdtype,
                           comm);
}

int barrier__ompi(MPI_Comm  comm)
{
    int communicator_size = comm->size();
    int alg = 1;
    int (*funcs[])(MPI_Comm) = {
        &barrier__ompi_basic_linear,
        &barrier__ompi_basic_linear,
        &barrier__ompi_recursivedoubling,
        &barrier__ompi_bruck,
        &barrier__ompi_two_procs,
        &barrier__ompi_tree
    };
    /** Algorithms:
     *  {1, "linear"},
     *  {2, "double_ring"},
     *  {3, "recursive_doubling"},
     *  {4, "bruck"},
     *  {5, "two_proc"},
     *  {6, "tree"},
     *
     * We can only optimize based on com size
     */
    if (communicator_size < 4) {
        alg = 3;
    } else if (communicator_size < 8) {
        alg = 1;
    } else if (communicator_size < 64) {
        alg = 3;
    } else if (communicator_size < 256) {
        alg = 4;
    } else if (communicator_size < 512) {
        alg = 6;
    } else if (communicator_size < 1024) {
        alg = 4;
    } else if (communicator_size < 4096) {
        alg = 6;
    } else {
        alg = 4;
    }

    return funcs[alg-1](comm);
}

int bcast__ompi(void *buff, int count, MPI_Datatype datatype, int root, MPI_Comm  comm)
{
    int alg = 1;
    size_t total_dsize, dsize;

    int communicator_size = comm->size();

    dsize = datatype->size();
    total_dsize = dsize * (unsigned long)count;
    int (*funcs[])(void*, int, MPI_Datatype, int, MPI_Comm) = {
        &bcast__NTSL,
        &bcast__ompi_pipeline,
        &bcast__ompi_pipeline,
        &bcast__ompi_split_bintree,
        &bcast__NTSB,
        &bcast__binomial_tree,
        &bcast__mvapich2_knomial_intra_node,
        &bcast__scatter_rdb_allgather,
        &bcast__scatter_LR_allgather,
    };
    /** Algorithms:
     *  {1, "basic_linear"},
     *  {2, "chain"},
     *  {3, "pipeline"},
     *  {4, "split_binary_tree"},
     *  {5, "binary_tree"},
     *  {6, "binomial"},
     *  {7, "knomial"},
     *  {8, "scatter_allgather"},
     *  {9, "scatter_allgather_ring"},
     */
    if (communicator_size < 4) {
        if (total_dsize < 32) {
            alg = 3;
        } else if (total_dsize < 256) {
            alg = 5;
        } else if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 1024) {
            alg = 7;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else if (total_dsize < 131072) {
            alg = 5;
        } else if (total_dsize < 262144) {
            alg = 2;
        } else if (total_dsize < 524288) {
            alg = 1;
        } else if (total_dsize < 1048576) {
            alg = 6;
        } else {
            alg = 5;
        }
    } else if (communicator_size < 8) {
        if (total_dsize < 64) {
            alg = 5;
        } else if (total_dsize < 128) {
            alg = 6;
        } else if (total_dsize < 2048) {
            alg = 5;
        } else if (total_dsize < 8192) {
            alg = 6;
        } else if (total_dsize < 1048576) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 16) {
        if (total_dsize < 8) {
            alg = 7;
        } else if (total_dsize < 64) {
            alg = 5;
        } else if (total_dsize < 4096) {
            alg = 7;
        } else if (total_dsize < 16384) {
            alg = 5;
        } else if (total_dsize < 32768) {
            alg = 6;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 32) {
        if (total_dsize < 4096) {
            alg = 7;
        } else if (total_dsize < 1048576) {
            alg = 6;
        } else {
            alg = 8;
        }
    } else if (communicator_size < 64) {
        if (total_dsize < 2048) {
            alg = 6;
        } else {
            alg = 7;
        }
    } else if (communicator_size < 128) {
        alg = 7;
    } else if (communicator_size < 256) {
        if (total_dsize < 2) {
            alg = 6;
        } else if (total_dsize < 16384) {
            alg = 5;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else if (total_dsize < 65536) {
            alg = 5;
        } else {
            alg = 7;
        }
    } else if (communicator_size < 1024) {
        if (total_dsize < 16384) {
            alg = 7;
        } else if (total_dsize < 32768) {
            alg = 4;
        } else {
            alg = 7;
        }
    } else if (communicator_size < 2048) {
        if (total_dsize < 524288) {
            alg = 7;
        } else {
            alg = 8;
        }
    } else if (communicator_size < 4096) {
        if (total_dsize < 262144) {
            alg = 7;
        } else {
            alg = 8;
        }
    } else {
        if (total_dsize < 8192) {
            alg = 7;
        } else if (total_dsize < 16384) {
            alg = 5;
        } else if (total_dsize < 262144) {
            alg = 7;
        } else {
            alg = 8;
        }
    }
    return funcs[alg-1](buff, count, datatype, root, comm);
}

int reduce__ompi(const void *sendbuf, void *recvbuf,
                 int count, MPI_Datatype  datatype,
                 MPI_Op   op, int root,
                 MPI_Comm   comm)
{
    size_t total_dsize, dsize;
    int alg = 1;
    int communicator_size = comm->size();

    dsize=datatype->size();
    total_dsize = dsize * count;
    int (*funcs[])(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm) = {
        &reduce__ompi_basic_linear,
        &reduce__ompi_chain,
        &reduce__ompi_pipeline,
        &reduce__ompi_binary,
        &reduce__ompi_binomial,
        &reduce__ompi_in_order_binary,
        //&reduce__rab our rab can't be used with all datatypes
        &reduce__ompi_basic_linear
    };
    /** Algorithms:
     *  {1, "linear"},
     *  {2, "chain"},
     *  {3, "pipeline"},
     *  {4, "binary"},
     *  {5, "binomial"},
     *  {6, "in-order_binary"},
     *  {7, "rabenseifner"},
     *
     * Currently, only linear and in-order binary tree algorithms are
     * capable of non commutative ops.
     */
     if ((op != MPI_OP_NULL) && not op->is_commutative()) {
        if (communicator_size < 4) {
            if (total_dsize < 8) {
                alg = 6;
            } else {
                alg = 1;
            }
        } else if (communicator_size < 8) {
            alg = 1;
        } else if (communicator_size < 16) {
            if (total_dsize < 1024) {
                alg = 6;
            } else if (total_dsize < 8192) {
                alg = 1;
            } else if (total_dsize < 16384) {
                alg = 6;
            } else if (total_dsize < 262144) {
                alg = 1;
            } else {
                alg = 6;
            }
        } else if (communicator_size < 128) {
            alg = 6;
        } else if (communicator_size < 256) {
            if (total_dsize < 512) {
                alg = 6;
            } else if (total_dsize < 1024) {
                alg = 1;
            } else {
                alg = 6;
            }
        } else {
            alg = 6;
        }
    } else {
        if (communicator_size < 4) {
            if (total_dsize < 8) {
                alg = 7;
            } else if (total_dsize < 16) {
                alg = 4;
            } else if (total_dsize < 32) {
                alg = 3;
            } else if (total_dsize < 262144) {
                alg = 1;
            } else if (total_dsize < 524288) {
                alg = 3;
            } else if (total_dsize < 1048576) {
                alg = 2;
            } else {
                alg = 3;
            }
        } else if (communicator_size < 8) {
            if (total_dsize < 4096) {
                alg = 4;
            } else if (total_dsize < 65536) {
                alg = 2;
            } else if (total_dsize < 262144) {
                alg = 5;
            } else if (total_dsize < 524288) {
                alg = 1;
            } else if (total_dsize < 1048576) {
                alg = 5;
            } else {
                alg = 1;
            }
        } else if (communicator_size < 16) {
            if (total_dsize < 8192) {
                alg = 4;
            } else {
                alg = 5;
            }
        } else if (communicator_size < 32) {
            if (total_dsize < 4096) {
                alg = 4;
            } else {
                alg = 5;
            }
        } else if (communicator_size < 256) {
            alg = 5;
        } else if (communicator_size < 512) {
            if (total_dsize < 8192) {
                alg = 5;
            } else if (total_dsize < 16384) {
                alg = 6;
            } else {
                alg = 5;
            }
        } else if (communicator_size < 2048) {
            alg = 5;
        } else if (communicator_size < 4096) {
            if (total_dsize < 512) {
                alg = 5;
            } else if (total_dsize < 1024) {
                alg = 6;
            } else if (total_dsize < 8192) {
                alg = 5;
            } else if (total_dsize < 16384) {
                alg = 6;
            } else {
                alg = 5;
            }
        } else {
            if (total_dsize < 16) {
                alg = 5;
            } else if (total_dsize < 32) {
                alg = 6;
            } else if (total_dsize < 1024) {
                alg = 5;
            } else if (total_dsize < 2048) {
                alg = 6;
            } else if (total_dsize < 8192) {
                alg = 5;
            } else if (total_dsize < 16384) {
                alg = 6;
            } else {
                alg = 5;
            }
        }
    }

    return funcs[alg-1] (sendbuf, recvbuf, count, datatype, op, root, comm);
}

int reduce_scatter__ompi(const void *sbuf, void *rbuf,
                         const int *rcounts,
                         MPI_Datatype dtype,
                         MPI_Op  op,
                         MPI_Comm  comm
                         )
{
    size_t total_dsize, dsize;
    int communicator_size = comm->size();
    int alg = 1;
    int zerocounts = 0;
    dsize=dtype->size();
    total_dsize = 0;
    for (int i = 0; i < communicator_size; i++) {
        total_dsize += rcounts[i];
       // if (0 == rcounts[i]) {
        //    zerocounts = 1;
        //}
    }
    total_dsize *= dsize;
    int (*funcs[])(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm) = {
        &reduce_scatter__default,
        &reduce_scatter__ompi_basic_recursivehalving,
        &reduce_scatter__ompi_ring,
        &reduce_scatter__ompi_ring,
    };
    /** Algorithms:
     *  {1, "non-overlapping"},
     *  {2, "recursive_halving"},
     *  {3, "ring"},
     *  {4, "butterfly"},
     *
     * Non commutative algorithm capability needs re-investigation.
     * Defaulting to non overlapping for non commutative ops.
     */
    if (((op != MPI_OP_NULL) && not op->is_commutative()) || (zerocounts)) {
        alg = 1;
    } else {
        if (communicator_size < 4) {
            if (total_dsize < 65536) {
                alg = 3;
            } else if (total_dsize < 131072) {
                alg = 4;
            } else {
                alg = 3;
            }
        } else if (communicator_size < 8) {
            if (total_dsize < 8) {
                alg = 1;
            } else if (total_dsize < 262144) {
                alg = 2;
            } else {
                alg = 3;
            }
        } else if (communicator_size < 32) {
            if (total_dsize < 262144) {
                alg = 2;
            } else {
                alg = 3;
            }
        } else if (communicator_size < 64) {
            if (total_dsize < 64) {
                alg = 1;
            } else if (total_dsize < 2048) {
                alg = 2;
            } else if (total_dsize < 524288) {
                alg = 4;
            } else {
                alg = 3;
            }
        } else if (communicator_size < 128) {
            if (total_dsize < 256) {
                alg = 1;
            } else if (total_dsize < 512) {
                alg = 2;
            } else if (total_dsize < 2048) {
                alg = 4;
            } else if (total_dsize < 4096) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else if (communicator_size < 256) {
            if (total_dsize < 256) {
                alg = 1;
            } else if (total_dsize < 512) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else if (communicator_size < 512) {
            if (total_dsize < 256) {
                alg = 1;
            } else if (total_dsize < 1024) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else if (communicator_size < 1024) {
            if (total_dsize < 512) {
                alg = 1;
            } else if (total_dsize < 2048) {
                alg = 2;
            } else if (total_dsize < 8192) {
                alg = 4;
            } else if (total_dsize < 16384) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else if (communicator_size < 2048) {
            if (total_dsize < 512) {
                alg = 1;
            } else if (total_dsize < 4096) {
                alg = 2;
            } else if (total_dsize < 16384) {
                alg = 4;
            } else if (total_dsize < 32768) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else if (communicator_size < 4096) {
            if (total_dsize < 512) {
                alg = 1;
            } else if (total_dsize < 4096) {
                alg = 2;
            } else {
                alg = 4;
            }
        } else {
            if (total_dsize < 1024) {
                alg = 1;
            } else if (total_dsize < 8192) {
                alg = 2;
            } else {
                alg = 4;
            }
        }
    }

    return funcs[alg-1] (sbuf, rbuf, rcounts, dtype, op, comm);
}

int allgather__ompi(const void *sbuf, int scount,
                    MPI_Datatype sdtype,
                    void* rbuf, int rcount,
                    MPI_Datatype rdtype,
                    MPI_Comm  comm
                    )
{
    int communicator_size;
    size_t dsize, total_dsize;
    int alg = 1;
    communicator_size = comm->size();
    if (MPI_IN_PLACE != sbuf) {
        dsize = sdtype->size();
    } else {
        dsize = rdtype->size();
    }
    total_dsize = dsize * (ptrdiff_t)scount;
    int (*funcs[])(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm) = {
        &allgather__NTSLR_NB,
        &allgather__bruck,
        &allgather__rdb,
        &allgather__ring,
        &allgather__ompi_neighborexchange,
        &allgather__pair
    };
    /** Algorithms:
     *  {1, "linear"},
     *  {2, "bruck"},
     *  {3, "recursive_doubling"},
     *  {4, "ring"},
     *  {5, "neighbor"},
     *  {6, "two_proc"}
     */
    if (communicator_size == 2) {
        alg = 6;
    } else if (communicator_size < 32) {
        alg = 3;
    } else if (communicator_size < 64) {
        if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 65536) {
            alg = 5;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 128) {
        if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 65536) {
            alg = 5;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 256) {
        if (total_dsize < 512) {
            alg = 3;
        } else if (total_dsize < 131072) {
            alg = 5;
        } else if (total_dsize < 524288) {
            alg = 4;
        } else if (total_dsize < 1048576) {
            alg = 5;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 512) {
        if (total_dsize < 32) {
            alg = 3;
        } else if (total_dsize < 128) {
            alg = 2;
        } else if (total_dsize < 1024) {
            alg = 3;
        } else if (total_dsize < 131072) {
            alg = 5;
        } else if (total_dsize < 524288) {
            alg = 4;
        } else if (total_dsize < 1048576) {
            alg = 5;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 1024) {
        if (total_dsize < 64) {
            alg = 3;
        } else if (total_dsize < 256) {
            alg = 2;
        } else if (total_dsize < 2048) {
            alg = 3;
        } else {
            alg = 5;
        }
    } else if (communicator_size < 2048) {
        if (total_dsize < 4) {
            alg = 3;
        } else if (total_dsize < 8) {
            alg = 2;
        } else if (total_dsize < 16) {
            alg = 3;
        } else if (total_dsize < 32) {
            alg = 2;
        } else if (total_dsize < 256) {
            alg = 3;
        } else if (total_dsize < 512) {
            alg = 2;
        } else if (total_dsize < 4096) {
            alg = 3;
        } else {
            alg = 5;
        }
    } else if (communicator_size < 4096) {
        if (total_dsize < 32) {
            alg = 2;
        } else if (total_dsize < 128) {
            alg = 3;
        } else if (total_dsize < 512) {
            alg = 2;
        } else if (total_dsize < 4096) {
            alg = 3;
        } else {
            alg = 5;
        }
    } else {
        if (total_dsize < 2) {
            alg = 3;
        } else if (total_dsize < 8) {
            alg = 2;
        } else if (total_dsize < 16) {
            alg = 3;
        } else if (total_dsize < 512) {
            alg = 2;
        } else if (total_dsize < 4096) {
            alg = 3;
        } else {
            alg = 5;
        }
    }

    return funcs[alg-1](sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);

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
    int alg = 1;
    communicator_size = comm->size();
    if (MPI_IN_PLACE != sbuf) {
        dsize = sdtype->size();
    } else {
        dsize = rdtype->size();
    }

    total_dsize = 0;
    for (i = 0; i < communicator_size; i++) {
        total_dsize += dsize * rcounts[i];
    }

    /* use the per-rank data size as basis, similar to allgather */
    size_t per_rank_dsize = total_dsize / communicator_size;

    int (*funcs[])(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm) = {
        &allgatherv__GB,
	&allgatherv__ompi_bruck,
	&allgatherv__mpich_ring,
	&allgatherv__ompi_neighborexchange,
	&allgatherv__pair
    };
    /** Algorithms:
     *  {1, "default"},
     *  {2, "bruck"},
     *  {3, "ring"},
     *  {4, "neighbor"},
     *  {5, "two_proc"},
     */
    if (communicator_size == 2) {
        if (per_rank_dsize < 2048) {
            alg = 3;
        } else if (per_rank_dsize < 4096) {
            alg = 5;
        } else if (per_rank_dsize < 8192) {
            alg = 3;
        } else {
            alg = 5;
        }
    } else if (communicator_size < 8) {
        if (per_rank_dsize < 256) {
            alg = 1;
        } else if (per_rank_dsize < 4096) {
            alg = 4;
        } else if (per_rank_dsize < 8192) {
            alg = 3;
        } else if (per_rank_dsize < 16384) {
            alg = 4;
        } else if (per_rank_dsize < 262144) {
            alg = 2;
        } else {
            alg = 4;
        }
    } else if (communicator_size < 16) {
        if (per_rank_dsize < 1024) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else if (communicator_size < 32) {
        if (per_rank_dsize < 128) {
            alg = 1;
        } else if (per_rank_dsize < 262144) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 64) {
        if (per_rank_dsize < 256) {
            alg = 1;
        } else if (per_rank_dsize < 8192) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 128) {
        if (per_rank_dsize < 256) {
            alg = 1;
        } else if (per_rank_dsize < 4096) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 256) {
        if (per_rank_dsize < 1024) {
            alg = 2;
        } else if (per_rank_dsize < 65536) {
            alg = 4;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 512) {
        if (per_rank_dsize < 1024) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 1024) {
        if (per_rank_dsize < 512) {
            alg = 2;
        } else if (per_rank_dsize < 1024) {
            alg = 1;
        } else if (per_rank_dsize < 4096) {
            alg = 2;
        } else if (per_rank_dsize < 1048576) {
            alg = 4;
        } else {
            alg = 3;
        }
    } else {
        if (per_rank_dsize < 4096) {
            alg = 2;
        } else {
            alg = 4;
        }
    }

    return funcs[alg-1](sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
}

int gather__ompi(const void *sbuf, int scount,
                 MPI_Datatype sdtype,
                 void* rbuf, int rcount,
                 MPI_Datatype rdtype,
                 int root,
                 MPI_Comm  comm
                 )
{
    int communicator_size, rank;
    size_t dsize, total_dsize;
    int alg = 1;
    communicator_size = comm->size();
    rank = comm->rank();

    if (rank == root) {
        dsize = rdtype->size();
        total_dsize = dsize * rcount;
    } else {
        dsize = sdtype->size();
        total_dsize = dsize * scount;
    }
    int (*funcs[])(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm) = {
        &gather__ompi_basic_linear,
	&gather__ompi_binomial,
	&gather__ompi_linear_sync
    };
    /** Algorithms:
     *  {1, "basic_linear"},
     *  {2, "binomial"},
     *  {3, "linear_sync"},
     *
     * We do not make any rank specific checks since the params
     * should be uniform across ranks.
     */
    if (communicator_size < 4) {
        if (total_dsize < 2) {
            alg = 3;
        } else if (total_dsize < 4) {
            alg = 1;
        } else if (total_dsize < 32768) {
            alg = 2;
        } else if (total_dsize < 65536) {
            alg = 1;
        } else if (total_dsize < 131072) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 8) {
        if (total_dsize < 1024) {
            alg = 2;
        } else if (total_dsize < 8192) {
            alg = 1;
        } else if (total_dsize < 32768) {
            alg = 2;
        } else if (total_dsize < 262144) {
            alg = 1;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 256) {
        alg = 2;
    } else if (communicator_size < 512) {
        if (total_dsize < 2048) {
            alg = 2;
        } else if (total_dsize < 8192) {
            alg = 1;
        } else {
            alg = 2;
        }
    } else {
        alg = 2;
    }

    return funcs[alg-1](sbuf, scount, sdtype, rbuf, rcount, rdtype, root, comm);
}


int scatter__ompi(const void *sbuf, int scount,
                  MPI_Datatype sdtype,
                  void* rbuf, int rcount,
                  MPI_Datatype rdtype,
                  int root, MPI_Comm  comm
                  )
{
    int communicator_size, rank;
    size_t dsize, total_dsize;
    int alg = 1;

    communicator_size = comm->size();
    rank = comm->rank();
    if (root == rank) {
        dsize=sdtype->size();
        total_dsize = dsize * scount;
    } else {
        dsize=rdtype->size();
        total_dsize = dsize * rcount;
    }
    int (*funcs[])(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm) = {
        &scatter__ompi_basic_linear,
        &scatter__ompi_binomial,
	&scatter__ompi_basic_linear
    };
    /** Algorithms:
     *  {1, "basic_linear"},
     *  {2, "binomial"},
     *  {3, "linear_nb"},
     *
     * We do not make any rank specific checks since the params
     * should be uniform across ranks.
     */
    if (communicator_size < 4) {
        if (total_dsize < 2) {
            alg = 3;
        } else if (total_dsize < 131072) {
            alg = 1;
        } else if (total_dsize < 262144) {
            alg = 3;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 8) {
        if (total_dsize < 2048) {
            alg = 2;
        } else if (total_dsize < 4096) {
            alg = 1;
        } else if (total_dsize < 8192) {
            alg = 2;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else if (total_dsize < 1048576) {
            alg = 3;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 16) {
        if (total_dsize < 16384) {
            alg = 2;
        } else if (total_dsize < 1048576) {
            alg = 3;
        } else {
            alg = 1;
        }
    } else if (communicator_size < 32) {
        if (total_dsize < 16384) {
            alg = 2;
        } else if (total_dsize < 32768) {
            alg = 1;
        } else {
            alg = 3;
        }
    } else if (communicator_size < 64) {
        if (total_dsize < 512) {
            alg = 2;
        } else if (total_dsize < 8192) {
            alg = 3;
        } else if (total_dsize < 16384) {
            alg = 2;
        } else {
            alg = 3;
        }
    } else {
        if (total_dsize < 512) {
            alg = 2;
        } else {
            alg = 3;
        }
    }

    return funcs[alg-1](sbuf, scount, sdtype, rbuf, rcount, rdtype, root, comm);
}

}
}
