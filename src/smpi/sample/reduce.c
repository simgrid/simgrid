#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv) {
  int size, rank;
  int root=0;
  int value = 1;
  int sum=-99;

  double start_timer;


  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  start_timer = MPI_Wtime();

  printf("rank %d has value %d\n", rank, value);
  MPI_Reduce(&value, &sum, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
  if ( rank == root) {
          printf("On root: sum=%d\n",sum);
	    printf("Elapsed time=%lf s\n", MPI_Wtime()-start_timer);
  }
  MPI_Finalize();
  return 0;
}
