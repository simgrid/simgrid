#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
  int size, rank;
  int value = 3;
  double start_timer;
  int quiet = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc > 1 && !strcmp(argv[1], "-q"))
    quiet = 1;

  start_timer = MPI_Wtime();

  if (0 == rank) {
    value = 17;
  }
  printf("node %d has value %d before broadcast\n", rank, value);
  MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
  printf("node %d has value %d after broadcast\n", rank, value);

  MPI_Barrier(MPI_COMM_WORLD);
  if (0 == rank && !quiet)
    printf("Elapsed time on rank %d: %lf s\n", rank,
           MPI_Wtime() - start_timer);
  MPI_Finalize();
  return 0;
}
