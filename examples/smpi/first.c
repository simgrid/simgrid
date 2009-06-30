#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int rank, size, err;

  err = MPI_Init(&argc, &argv); /* Initialize MPI */
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }

  err = MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (err != MPI_SUCCESS) {
    printf("MPI Get Communicator Size Failed!\n");
  }

  err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (err != MPI_SUCCESS) {
    printf("MPI Get Communicator Rank Failed!\n");
  }

  if (0 == rank) {
    printf("root node believes there are %d nodes in world.\n", size);
  }

  sleep(20);

  err = MPI_Finalize();         /* Terminate MPI */
  return 0;
}
