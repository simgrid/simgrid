#include <stdio.h>
#include <mpi.h>

/**
 * MESSAGE PASSING INTERFACE TEST CASE SUITE
 *
 * Copyright IBM Corp. 1995
 * 
 * IBM Corp. hereby grants a non-exclusive license to use, copy, modify, and
 *distribute this software for any purpose and without fee provided that the
 *above copyright notice and the following paragraphs appear in all copies.

 * IBM Corp. makes no representation that the test cases comprising this
 * suite are correct or are an accurate representation of any standard.

 * In no event shall IBM be liable to any party for direct, indirect, special
 * incidental, or consequential damage arising out of the use of this software
 * even if IBM Corp. has been advised of the possibility of such damage.

 * IBM CORP. SPECIFICALLY DISCLAIMS ANY WARRANTIES INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS AND IBM
 * CORP. HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
 * ENHANCEMENTS, OR MODIFICATIONS.
 * ***************************************************************************
 **/
static int ibm_test(int rank, int size)
{
  int success = 1;
#define MAXLEN  10000

  int root, i, j, k;
  int out[MAXLEN];
  int in[MAXLEN];
  root = size / 2;

  for (j = 1; j <= MAXLEN; j *= 10) {
    for (i = 0; i < j; i++)
      out[i] = i;

    MPI_Reduce(out, in, j, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);

    if (rank == root) {
      for (k = 0; k < j; k++) {
        if (in[k] != k * size) {
          printf("bad answer (%d) at index %d of %d (should be %d)", in[k], k,
                 j, k * size);
          success = 0;
          break;
        }
      }
    }
  }
  return (success);
}




int main(int argc, char **argv)
{
  int size, rank;
  int root = 0;
  int value;
  int sum = -99, sum_mirror = -99, min = 999, max = -999;

  double start_timer;
  int quiet = 0;


  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc > 1 && !strcmp(argv[1], "-q"))
    quiet = 1;

  start_timer = MPI_Wtime();

  value = rank + 1;             /* easy to verify that sum= (size*(size+1))/2; */

  //printf("[%d] has value %d\n", rank, value);
  MPI_Reduce(&value, &sum, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
  MPI_Reduce(&value, &sum_mirror, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);

  MPI_Reduce(&value, &min, 1, MPI_INT, MPI_MIN, root, MPI_COMM_WORLD);
  MPI_Reduce(&value, &max, 1, MPI_INT, MPI_MAX, root, MPI_COMM_WORLD);
  if (rank == root) {
    printf("** Scalar Int Test Result:\n");
    printf("\t[%d] sum=%d ... validation ", rank, sum);
    if (((size * (size + 1)) / 2 == sum) && (sum_mirror == sum))
      printf("ok.\n");
    else
      printf("failed (sum=%d,sum_mirror=%d while both sould be %d.\n",
             sum, sum_mirror, (size * (size + 1)) / 2);
    printf("\t[%d] min=%d ... validation ", rank, min);
    if (1 == min)
      printf("ok.\n");
    else
      printf("failed.\n");
    printf("\t[%d] max=%d ... validation ", rank, max);
    if (size == max)
      printf("ok.\n");
    else
      printf("failed.\n");
    if (!quiet)
      printf("Elapsed time=%lf s\n", MPI_Wtime() - start_timer);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (0 == rank)
    printf("** IBM Test Result: ... \n");
  if (!ibm_test(rank, size))
    printf("\t[%d] failed.\n", rank);
  else if (!quiet)
    printf("\t[%d] ok.\n", rank);
  else
    printf("\tok.\n");

  MPI_Finalize();
  return 0;
}
