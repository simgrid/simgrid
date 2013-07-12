/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Regression test for ticket #1785, contributed by Jed Brown.  The test was
 * hanging indefinitely under a buggy version of ch3:sock. */

#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#if !defined(USE_STRICT_MPI) && defined(MPICH)
#define TEST_NBC_ROUTINES 1
#endif

int main(int argc, char *argv[])
{
    MPI_Request barrier;
    int rank,i,done;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    #if defined(TEST_NBC_ROUTINES)
    MPI_Ibarrier(MPI_COMM_WORLD,&barrier);
    for (i=0,done=0; !done; i++) {
        usleep(1000);
        /*printf("[%d] MPI_Test: %d\n",rank,i);*/
        MPI_Test(&barrier,&done,MPI_STATUS_IGNORE);
    }
    #endif
    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
