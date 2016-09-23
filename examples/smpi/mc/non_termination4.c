#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int x = 20;

int main(int argc, char **argv) {
  int recv_x = 1, size, rank;
  MPI_Status status;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */

  MC_ignore(&(status.count), sizeof(status.count));

  if(rank==0){
    while (recv_x>=0) {
      MPI_Recv(&recv_x, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
  }else{
    while (x >= 0) {
      if (MC_random(0,1) == 0) {
        x -= 1;
      } else {
        x += 1;
      }
      printf("x=%d\n", x);
      MPI_Send(&x, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();

  return 0;
}
