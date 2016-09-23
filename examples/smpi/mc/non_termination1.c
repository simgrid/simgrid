#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int x = 5;
int y = 8;

int main(int argc, char **argv) {
  int recv_buff, size, rank;
  MPI_Status status;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */

  MC_ignore(&(status.count), sizeof(status.count));

  if (rank == 0) {
    while (1) {
      MPI_Recv(&recv_buff, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
  } else {
    while (1) {
      int old_x = x;
      x = -y;
      y = old_x;
      printf("x = %d, y = %d\n", x, y);
      MPI_Send(&rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();

  return 0;
}
