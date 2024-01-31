#include <assert.h>
#include <mpi.h>
#include <stdio.h>

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 1024
#endif

int main(int argc, char** argv)
{
  int nprocs = -1;
  int rank   = -1;

  int const0 = 0;
  int const1 = 1;
  int trash;

  MPI_Status status;
  MPI_Request request[2];

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (nprocs < 3) {
    printf("\033[0;31m! This test needs at least 3 processes !\033[0;0m\n");
    MPI_Finalize();
    return 1;
  }

  if (rank == 0) { // dummy receiver

    int g = 0;

    MPI_Irecv(&g, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &(request[0]));
    MPI_Irecv(&g, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &(request[1]));
    MPI_Waitany(2, request, &trash, &status);
    MPI_Waitany(2, request, &trash, &status);
    assert(g != 1);
  }

  if (rank == 1) {

    MPI_Send(&const1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  if (rank == 2) {

    MPI_Send(&const0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
