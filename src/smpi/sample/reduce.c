#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
  int rank, size;
  int i;
  int *sendbuf, *recvbuf;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  sendbuf = malloc(sizeof(int) * size);
  recvbuf = malloc(sizeof(int) * size);
  for (i = 0; i < size; i++) {
    sendbuf[i] = 0;
    recvbuf[i] = 0;
  }
  sendbuf[rank] = rank + 1;
  MPI_Reduce(sendbuf, recvbuf, size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  if (0 == rank) {
    printf("nodes: ", rank);
    for (i = 0; i < size; i++) {
      printf("%d ", recvbuf[i]);
    }
    printf("\n");
  }
  free(sendbuf);
  free(recvbuf);
  MPI_Finalize();
  return 0;
}
