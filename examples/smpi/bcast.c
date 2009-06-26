#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv) {
  int size, rank;
  int value = 3;
  double start_timer;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  start_timer = MPI_Wtime();

  if (0 == rank) {
    value = 17;
  }
  printf("node %d has value %d\n", rank, value);
  MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
  printf("node %d has value %d\n", rank, value);

  MPI_Barrier( MPI_COMM_WORLD );
  if ( 0 == rank)
	    printf("Elapsed time on rank %d: %lf s\n", rank, MPI_Wtime()-start_timer);
  MPI_Finalize();
  return 0;
}
