#include <stdio.h>
#include <mpi.h>

#define INIT_VALUE 3
#define TARGET_VALUE 42

int main (int argc, char **argv) {
  int size, rank;
  int value = INIT_VALUE;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (0 == rank) {
    value = TARGET_VALUE;
  }
  fprintf(stderr,"node %d has value %d before broadcast\n", rank, value);
  MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
  fprintf(stderr,"node %d has value %d after broadcast\n", rank, value);
  if (value != TARGET_VALUE) {
	  fprintf(stderr,"node %d don't have the target value after broadcast!!\n", rank);
	  exit(1);
  }
  MPI_Finalize();
  return 0;
}
