#include "mpi.h"
#include <stdio.h>
#include <string.h>

#define BUFSIZE (1024 * 1024)
#define BOUNDED(sz) ((sz) < BUFSIZE ? (sz) : BUFSIZE)

static void setup_recvbuf(int nprocs, int** recvbuf, int** displs, int** counts, int** rcounts)
{
  *recvbuf = xbt_malloc(BUFSIZE * nprocs * sizeof(int));
  for (int i = 0; i < BUFSIZE * nprocs; i++)
    (*recvbuf)[i] = i;

  *displs  = xbt_malloc(nprocs * sizeof(int));
  *counts  = xbt_malloc(nprocs * sizeof(int));
  *rcounts = xbt_malloc(nprocs * sizeof(int));
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

  int* sendbuf = xbt_malloc(BUFSIZE * nprocs * sizeof(int));
  for (int i = 0; i < BUFSIZE * nprocs; i++)
    sendbuf[i] = rank;

  int* alltoallvcounts = xbt_malloc(nprocs * sizeof(int));
  for (int i = 0; i < nprocs; i++)
    alltoallvcounts[i] = BOUNDED(i + rank);

  int* dummy_buffer = xbt_malloc(sizeof(int));
  // initialize buffers with an invalid value (we want to trigger a valgrind error if they are used)
  int* recvbuf      = dummy_buffer + 1;
  int* displs       = dummy_buffer + 1;
  int* counts       = dummy_buffer + 1;
  int* rcounts      = dummy_buffer + 1;
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

  xbt_free(dummy_buffer);
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

  xbt_free(alltoallvcounts);
  xbt_free(sendbuf);
  xbt_free(recvbuf);
  xbt_free(displs);
  xbt_free(counts);
  xbt_free(rcounts);

  MPI_Finalize();
  return 0;
}
