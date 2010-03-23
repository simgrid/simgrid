#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv) {
  int size, rank;

  char name[MPI_MAX_PROCESSOR_NAME];
  int len;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Get_processor_name(name,&len);
  printf("rank %d is running on host %s\n", rank, name);
  MPI_Finalize();
  return 0;
}
