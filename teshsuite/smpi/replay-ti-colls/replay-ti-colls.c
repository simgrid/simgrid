//exclude from clang static analysis, as there is an intentional uninitialized value passed to MPI calls.
#ifndef __clang_analyzer__

#include "mpi.h"
#include <stdio.h>
#include <string.h>

#define BUFSIZE (1024 * 1024)
#define BOUNDED(sz) ((sz) < BUFSIZE ? (sz) : BUFSIZE)

static void setup_recvbuf(int nprocs, int** recvbuf, int** displs, int** counts, int** rcounts)
{
  *recvbuf = malloc(BUFSIZE * nprocs * sizeof(int));
  for (int i = 0; i < BUFSIZE * nprocs; i++)
    (*recvbuf)[i] = i;

  *displs  = malloc(nprocs * sizeof(int));
  *counts  = malloc(nprocs * sizeof(int));
  *rcounts = malloc(nprocs * sizeof(int));
  for (int i = 0; i < nprocs; i++) {
    (*displs)[i]  = i * BUFSIZE;
    (*counts)[i]  = BOUNDED(i);
    (*rcounts)[i] = (*counts)[i];
  }
}

int main(int argc, char** argv)
{
  int nprocs = -1;
  int rank   = -1;

  /* init */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int* sendbuf = malloc(BUFSIZE * nprocs * sizeof(int));
  for (int i = 0; i < BUFSIZE * nprocs; i++)
    sendbuf[i] = rank;

  int* alltoallvcounts = malloc(nprocs * sizeof(int));
  for (int i = 0; i < nprocs; i++)
    alltoallvcounts[i] = BOUNDED(i + rank);

  int* recvbuf;
  int* displs;
  int* counts;
  int* rcounts;
  if (rank == 0)
    setup_recvbuf(nprocs, &recvbuf, &displs, &counts, &rcounts);

  // first test, with unallocated non significative buffers
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gather(&sendbuf[rank * BUFSIZE], BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatter(recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gatherv(&sendbuf[rank * BUFSIZE], BOUNDED(rank), MPI_INT, recvbuf, rcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv(recvbuf, counts, displs, MPI_INT, sendbuf, BOUNDED(rank), MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Reduce(sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  if (rank != 0)
    setup_recvbuf(nprocs, &recvbuf, &displs, &counts, &rcounts);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gather(&sendbuf[rank * BUFSIZE], BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatter(recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gatherv(&sendbuf[rank * BUFSIZE], BOUNDED(rank), MPI_INT, recvbuf, rcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv(recvbuf, counts, displs, MPI_INT, sendbuf, BOUNDED(rank), MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Reduce(sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Allgather(sendbuf, BUFSIZE, MPI_INT, recvbuf, BUFSIZE, MPI_INT, MPI_COMM_WORLD);
  MPI_Alltoall(recvbuf, BUFSIZE, MPI_INT, sendbuf, BUFSIZE, MPI_INT, MPI_COMM_WORLD);
  MPI_Allgatherv(sendbuf, BOUNDED(rank), MPI_INT, recvbuf, rcounts, displs, MPI_INT, MPI_COMM_WORLD);
  MPI_Alltoallv(recvbuf, alltoallvcounts, displs, MPI_INT, sendbuf, alltoallvcounts, displs, MPI_INT, MPI_COMM_WORLD);
  MPI_Allreduce(sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Reduce_scatter(sendbuf, recvbuf, rcounts, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Scan(sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Exscan(sendbuf, recvbuf, BUFSIZE, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

  free(alltoallvcounts);
  free(sendbuf);
  free(recvbuf);
  free(displs);
  free(counts);
  free(rcounts);

  MPI_Finalize();
  return 0;
}

#endif
