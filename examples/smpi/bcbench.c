#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define GETTIMEOFDAY_ERROR 1

#define N_START 1
#define N_STOP  1024*1024
#define N_NEXT  (N*2)
#define ITER    100
#define ONE_MILLION 1000000.0
#define RAND_SEED 842270

int main(int argc, char *argv[])
{

  int size, rank;
  int N;
  struct timeval *start_time, *stop_time;
  double seconds;
  int i, j;
  char *buffer;
  int check;

  srandom(RAND_SEED);

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (0 == rank) {
    start_time = (struct timeval *) malloc(sizeof(struct timeval));
    stop_time = (struct timeval *) malloc(sizeof(struct timeval));
  }

  for (N = N_START; N <= N_STOP; N = N_NEXT) {

    buffer = malloc(sizeof(char) * N);

    if (0 == rank) {
      for (j = 0; j < N; j++) {
        buffer[j] = (char) (random() % 256);
      }
      if (-1 == gettimeofday(start_time, NULL)) {
        printf("couldn't set start_time on node 0!\n");
        MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
        exit(EXIT_FAILURE);
      }
    }

    for (i = 0; i < ITER; i++) {
      MPI_Bcast(buffer, N, MPI_BYTE, 0, MPI_COMM_WORLD);
      if (0 == rank) {
        for (j = 1; j < size; j++) {
          MPI_Recv(&check, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD,
                   MPI_STATUS_IGNORE);
        }
      } else {
        MPI_Send(&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
      }
    }

    if (0 == rank) {
      if (-1 == gettimeofday(stop_time, NULL)) {
        printf("couldn't set start_time on node 0!\n");
        MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
        exit(EXIT_FAILURE);
      }
      seconds =
        (double) (stop_time->tv_sec - start_time->tv_sec) +
        (double) (stop_time->tv_usec - start_time->tv_usec) / ONE_MILLION;
    }

    free(buffer);

    if (0 == rank) {
      printf("N: %10d, iter: %d, time: %10f s, avg rate: %12f Mbps\n", N,
             ITER, seconds,
             ((double) N * ITER * 8) / (1024.0 * 1024.0 * seconds));
    }

  }

  if (0 == rank) {
    free(start_time);
    free(stop_time);
  }

  MPI_Finalize();

  return 0;
}
