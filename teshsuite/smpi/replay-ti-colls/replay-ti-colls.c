//exclude from clang static analysis, as there is an intentional uninitialized value passed to MPI calls.
#ifndef __clang_analyzer__

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define BUFSIZE 1024*1024

int
main (int argc, char **argv){
  int i, nprocs = -1;
  int rank = -1;
  int *sendbuf, *recvbuf, *displs, *counts, *rcounts, *alltoallvcounts;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  sendbuf = (int *) malloc (BUFSIZE * nprocs * sizeof(int));
  for (i = 0; i < BUFSIZE * nprocs; i++)
    sendbuf[i] = rank;

  alltoallvcounts = (int *) malloc (nprocs * sizeof(int));
  for (i = 0; i < nprocs; i++)
    if ((i + rank) < BUFSIZE)
      alltoallvcounts[i] = i + rank;
    else
      alltoallvcounts[i] = BUFSIZE;

  if (rank == 0) {
    recvbuf = (int *) malloc (BUFSIZE * nprocs * sizeof(int));
    for (i = 0; i < BUFSIZE * nprocs; i++)
      recvbuf[i] = i;

    displs = (int *) malloc (nprocs * sizeof(int));
    counts = (int *) malloc (nprocs * sizeof(int));
    rcounts = (int *) malloc (nprocs * sizeof(int));
    for (i = 0; i < nprocs; i++) {
      displs[i] = i * BUFSIZE;
      if (i < BUFSIZE)
        rcounts[i] = counts[i] = i;
      else
        rcounts[i] = counts[i] = BUFSIZE;
    }
  }

  //first test, with unallocated non significative buffers
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Bcast (sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gather (&sendbuf[rank*BUFSIZE], BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatter (recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gatherv (&sendbuf[rank*BUFSIZE], (rank < BUFSIZE) ? rank : BUFSIZE, MPI_INT, recvbuf, rcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv (recvbuf, counts, displs, MPI_INT, sendbuf, (rank < BUFSIZE) ? rank : BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Reduce (sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  if (rank != 0) {
    recvbuf = (int *) malloc (BUFSIZE * nprocs * sizeof(int));
    for (i = 0; i < BUFSIZE * nprocs; i++)
      recvbuf[i] = i;

    displs = (int *) malloc (nprocs * sizeof(int));
    counts = (int *) malloc (nprocs * sizeof(int));
    rcounts = (int *) malloc (nprocs * sizeof(int));
    for (i = 0; i < nprocs; i++) {
      displs[i] = i * BUFSIZE;
      if (i < BUFSIZE)
        rcounts[i] = counts[i] = i;
      else
        rcounts[i] = counts[i] = BUFSIZE;
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Bcast (sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gather (&sendbuf[rank*BUFSIZE], BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatter (recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gatherv (&sendbuf[rank*BUFSIZE], (rank < BUFSIZE) ? rank : BUFSIZE, MPI_INT, recvbuf, rcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv (recvbuf, counts, displs, MPI_INT, sendbuf, (rank < BUFSIZE) ? rank : BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Reduce (sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Allgather (sendbuf, BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, MPI_COMM_WORLD);
  MPI_Alltoall (recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, MPI_COMM_WORLD);
  MPI_Allgatherv (sendbuf, (rank < BUFSIZE) ? rank : BUFSIZE, MPI_INT, recvbuf, rcounts, displs, MPI_INT, MPI_COMM_WORLD);
  MPI_Alltoallv (recvbuf, alltoallvcounts, displs, MPI_INT, sendbuf, alltoallvcounts, displs, MPI_INT, MPI_COMM_WORLD);
  MPI_Allreduce (sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Reduce_scatter (sendbuf, recvbuf, rcounts, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Scan (sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Exscan (sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Barrier (MPI_COMM_WORLD);

  free (alltoallvcounts);
  free (sendbuf);
  free (recvbuf);
  free (displs);
  free (counts);
  free (rcounts);

  MPI_Finalize ();
  return 0;
}

#endif