/*
 * Mark Stillwell
 * ICS691: High Performance Computing
 * Fall 2006
 * Homework 3, Exercise 2, Step 1
 */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <mpi.h>

#define ITERATIONS         10
#define STEPS              1
#define STEP_SIZE          0

#define USAGE_ERROR        1
#define MALLOC_ERROR       2
#define GETTIMEOFDAY_ERROR 3

void * checked_malloc(int rank, const char * varname, size_t size) {
  void * ptr;
  ptr = malloc(size);
  if (NULL == ptr) {
    printf("node %d could not malloc memory for %s.\n", rank, varname);
    MPI_Abort(MPI_COMM_WORLD, MALLOC_ERROR);
    exit(MALLOC_ERROR);
  }
  return ptr;
}

int main(int argc, char* argv[]) {

  // timing/system variables
  int iteration, iterations = ITERATIONS;
  int step, steps = STEPS, step_size = STEP_SIZE;
  long usecs, total_usecs;
  struct timeval *start_time, *stop_time;
  char *program;

  // mpi/communications variables
  int rank;
  int row, col;
  MPI_Comm row_comm, col_comm;

  // algorithm variables
  int N_start, N, P;
  int *A, *A_t, *B, *C, *D, *a, *b, *abuf, *bbuf;
  int n, i, j, k, I, J;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (0 == rank) {
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    program = basename(argv[0]);

    // root node parses cmdline args
    /*
    if (3 > argc || !isdigit(*argv[1]) || !isdigit(*argv[2])) {
      printf("usage:\n%s <N> <P> [<iterations>]\n", program);
      MPI_Abort(MPI_COMM_WORLD, USAGE_ERROR);
      exit(USAGE_ERROR);
    }
    */

    //N_start = atoi(argv[1]);
    //P = atoi(argv[2]);
    N_start = 100;
    P = 2;

    /*
    if (4 <= argc && isdigit(*argv[3])) {
      iterations = atoi(argv[3]);
    }

    if (5 <= argc && isdigit(*argv[4])) {
      steps = atoi(argv[4]);
    }

    if (6 <= argc && isdigit(*argv[5])) {
      step_size = atoi(argv[5]);
    }
    */

    if (P*P != size) {
      printf("P^2 must equal size.\n");
      MPI_Abort(MPI_COMM_WORLD, USAGE_ERROR);
      exit(USAGE_ERROR);
    }

    start_time = (struct timeval *)checked_malloc(rank, "start_time", sizeof(struct timeval));
    stop_time  = (struct timeval *)checked_malloc(rank, "stop_time",  sizeof(struct timeval));

  }

  // send command line parameters except N, since it can vary
  MPI_Bcast(&P, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&step_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

  row = rank / P;
  col = rank % P;

  // create row/column communicators
  MPI_Comm_split(MPI_COMM_WORLD, row, col, &row_comm);
  MPI_Comm_split(MPI_COMM_WORLD, col, row, &col_comm);

  for (step = 0; step < steps; step++) {

    total_usecs = 0;

    if (0 == rank) {
      N = N_start + step * step_size;
      if ((N/P)*P != N) {
        printf("P must divide N and %d does not divide %d.\n", P, N);
        N = -1;
      }
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // if root passes N = -1, skip this round
    if (-1 == N) continue;

    n = N / P;

    // initialize matrix components
    A   = (int *)checked_malloc(rank, "A",   n*n*sizeof(int));
    A_t = (int *)checked_malloc(rank, "A_t", n*n*sizeof(int));
    B   = (int *)checked_malloc(rank, "B",   n*n*sizeof(int));
    C   = (int *)checked_malloc(rank, "C",   n*n*sizeof(int));
    D   = (int *)checked_malloc(rank, "D",   n*n*sizeof(int));

    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {

        I = n*row+i;
        J = n*col+j;

        A[n*i+j] = I+J;
        B[n*i+j] = I;

        // d is the check matrix
        D[n*i+j] = 0;
        for (k = 0; k < N; k++) {
          // A[I,k] = I+k
          // B[k,J] = k
          D[n*i+j] += (I+k) * k;
        }

      }
    }

    // buffers
    abuf = (int *)checked_malloc(rank, "abuf", n*sizeof(int));
    bbuf = (int *)checked_malloc(rank, "bbuf", n*sizeof(int));

    for (iteration = 0; iteration < iterations; iteration++) {

      for (i = 0; i < n*n; i++) {
        C[i] = 0;
      }

      // node zero sets start time
      if (0 == rank && -1 == gettimeofday(start_time, NULL)) {
        printf("couldn't set start_time on node 0!\n");
        MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
        exit(GETTIMEOFDAY_ERROR);
      }

      // populate transpose of A
      for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
          A_t[n*i+j] = A[n*j+i];
        }
      }

      // perform calculations
      for (k = 0; k < N; k++) {

        if (k/n == col) {
          a = A_t + n*(k%n);
        } else {
          a = abuf;
        }

        if (k/n == row) {
          b = B + n*(k%n);
        } else {
          b = bbuf;
        }

        MPI_Bcast(a, n, MPI_INT, k/n, row_comm);
        MPI_Bcast(b, n, MPI_INT, k/n, col_comm);

        for (i = 0; i < n; i++) {
          for (j = 0; j < n; j++) {
            C[n*i+j] += a[i] * b[j];
          }
        }

      } // for k

      // node zero sets stop time
      if (0 == rank && -1 == gettimeofday(stop_time, NULL)) {
        printf("couldn't set stop_time on node 0!\n");
        MPI_Abort(MPI_COMM_WORLD, GETTIMEOFDAY_ERROR);
        exit(GETTIMEOFDAY_ERROR);
      }

      // check calculation
      for (i = 0; i < n*n && C[i] == D[i]; i++);
      j = (n*n == i);
      MPI_Reduce(&j, &k, 1, MPI_INT, MPI_LAND, 0, MPI_COMM_WORLD);

      // node zero prints stats
      if (0 == rank) {
        usecs = (stop_time->tv_sec*1000000+stop_time->tv_usec) - (start_time->tv_sec*1000000+start_time->tv_usec);
        printf("prog: %s, N: %d, P: %d, procs: %d, time: %d us, check: %d\n", program, N, P, P*P, usecs, k);
        total_usecs += usecs;
      }

    }

    // node 0 prints final stats
    if (0 == rank) {
      printf("prog: %s, N: %d, P: %d, procs: %d, iterations: %d, avg. time: %d us\n",
          program, N, P, P*P, iterations, total_usecs / iterations);
    }

    // free data structures
    free(A);
    free(A_t);
    free(B);
    free(C);
    free(D);
    free(abuf);
    free(bbuf);

  }

  if (0 == rank) {
    free(start_time);
    free(stop_time);
  }

  MPI_Finalize();

  return 0;
}
