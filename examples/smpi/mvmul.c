#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS         10

#define USAGE_ERROR        1
#define SANITY_ERROR       2
#define GETTIMEOFDAY_ERROR 3

int main(int argc, char* argv[]) {

  int size, rank;
  int N, n, i, j, k, current_iteration, successful_iterations = 0;
  double *matrix, *vector, *vcalc, *vcheck;
  MPI_Status status;
  struct timeval *start_time, *stop_time;
  long parallel_usecs, parallel_usecs_total = 0, sequential_usecs, sequential_usecs_total = 0;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (0 == rank) {

    // root node parses cmdline args
    if (2 > argc || !isdigit(*argv[1])) {
      printf("usage:\n%s <size>\n", argv[0]);
      MPI_Abort(MPI_COMM_WORLD, USAGE_ERROR);
      exit(USAGE_ERROR);
    }

    N = atoi(argv[1]);

    start_time     = (struct timeval *)malloc(sizeof(struct timeval));
    stop_time      = (struct timeval *)malloc(sizeof(struct timeval));

  }

  for(current_iteration = 0; current_iteration < ITERATIONS; current_iteration++) {

    if (0 == rank) {

      matrix         = (double *)malloc(N*N*sizeof(double));
      vector         = (double *)malloc(N*sizeof(double));

      for(i = 0; i < N*N; i++) {
        matrix[i] = (double)rand()/((double)RAND_MAX + 1);
      }

      for(i = 0; i < N; i++) {
        vector[i] = (double)rand()/((double)RAND_MAX + 1);
      }

      // for the sake of argument, the parallel algorithm begins
      // when the root node begins to transmit the matrix to the
      // workers.
      if (-1 == gettimeofday(start_time, NULL)) {
        printf("couldn't set start_time on node 0!\n");
        MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
        exit(GETTIMEOFDAY_ERROR);
      }

      for(i = 1; i < size; i++) {
        MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }

    } else {
      MPI_Recv(&N, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }

    // this algorithm uses at most N processors...
    if (rank < N) {

      if (size > N) size = N;
      n = N / size + ((rank < (N % size)) ? 1 : 0);

      if (0 == rank) {

        for(i = 1, j = n; i < size && j < N; i++, j+=k) {
          k = N / size + ((i < (N % size)) ? 1 : 0);
          MPI_Send(matrix+N*j, N*k, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
          MPI_Send(vector, N, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        // sanity check
        #ifdef DEBUG
        if(i != size || j != N) {
          printf("index calc error: i = %d, size = %d, j = %d, N = %d\n", i, size, j, N);
          MPI_Abort(MPI_COMM_WORLD, SANITY_ERROR);
          exit(SANITY_ERROR);
        }
        #endif

        vcalc = (double *)malloc(N*sizeof(double));

      } else {

        matrix = (double *)malloc(N*n*sizeof(double));
        vector = (double *)malloc(N*sizeof(double));
        vcalc  = (double *)malloc(n*sizeof(double));

        MPI_Recv(matrix, N*n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(vector, N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);

      }

      for(i = 0; i < n; i++) {
        vcalc[i] = 0.0;
        for(j = 0; j < N; j++) {
          vcalc[i] += matrix[N*i+j] * vector[j];
        }
      }

      if (0 != rank) {
        MPI_Send(vcalc, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      } else {

        for(i = 1, j = n; i < size && j < N; i++, j+=k) {
          k = N / size + ((i < (N % size)) ? 1 : 0);
          MPI_Recv(vcalc+j, k, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
        }

        // sanity check
        #ifdef DEBUG
        if(i != size || j != N) {
          printf("index calc error 2: i = %d, size = %d, j = %d, N = %d\n", i, size, j, N);
          MPI_Abort(MPI_COMM_WORLD, SANITY_ERROR);
          exit(SANITY_ERROR);
        }
        #endif

        if (-1 == gettimeofday(stop_time, NULL)) {
          printf("couldn't set stop_time on node 0!\n");
          MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
          exit(GETTIMEOFDAY_ERROR);
        }

        parallel_usecs = (stop_time->tv_sec*1000000+stop_time->tv_usec) - (start_time->tv_sec*1000000+start_time->tv_usec);

        if (-1 == gettimeofday(start_time, NULL)) {
          printf("couldn't set start_time on node 0!\n");
          MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
          exit(GETTIMEOFDAY_ERROR);
        }

        // calculate serially
        vcheck = (double *)malloc(N*sizeof(double));
        for(i = 0; i < N; i++) {
          vcheck[i] = 0.0;
          for(j = 0; j < N; j++) {
            vcheck[i] += matrix[N*i+j] * vector[j];
          }
        }

        if (-1 == gettimeofday(stop_time, NULL)) {
          printf("couldn't set stop_time on node 0!\n");
          MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
          exit(GETTIMEOFDAY_ERROR);
        }

        sequential_usecs = (stop_time->tv_sec*1000000+stop_time->tv_usec) - (start_time->tv_sec*1000000+start_time->tv_usec);

        // verify correctness
        for(i = 0; i < N && vcalc[i] == vcheck[i]; i++);
        
        printf("prog: blocking, i: %d ", current_iteration);

        if (i == N) {
          printf("ptime: %ld us, stime: %ld us, speedup: %.3f, nodes: %d, efficiency: %.3f\n",
              parallel_usecs,
              sequential_usecs,
              (double)sequential_usecs / (double)parallel_usecs,
              size,
              (double)sequential_usecs / ((double)parallel_usecs * (double)size));

          parallel_usecs_total += parallel_usecs;
          sequential_usecs_total += sequential_usecs;
          successful_iterations++;
        } else {
          printf("parallel calc != serial calc, ");
        }

        free(vcheck);

      }

      free(matrix);
      free(vector);
      free(vcalc);
    }

  }

  if(0 == rank) {
    printf("prog: blocking, ");
    if(0 < successful_iterations) {
      printf("iterations: %d, avg. ptime: %.3f us, avg. stime: %.3f us, avg. speedup: %.3f, nodes: %d, avg. efficiency: %.3f\n",
          successful_iterations,
          (double) parallel_usecs_total / (double) successful_iterations,
          (double) sequential_usecs_total / (double) successful_iterations,
          (double)sequential_usecs_total / (double)parallel_usecs_total,
          size,
          (double)sequential_usecs_total / ((double)parallel_usecs_total * (double)size));
    } else {
      printf("no successful iterations!\n");
    }

    free(start_time);
    free(stop_time);

  }

  MPI_Finalize();

  return 0;
}
