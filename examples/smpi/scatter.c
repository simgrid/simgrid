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
#include <stdio.h>
#include <mpi.h>

static int ibm_test(int rank, int size)
{

#define MAXLEN  10000

  int success = 1;
  int root = 0;
  int i, j, k;
  int *out;
  int *in;


  out = malloc(MAXLEN * 64 * sizeof(int));
  in = malloc(MAXLEN * sizeof(int));

  for (j = 1; j <= MAXLEN; j *= 10) {
    root = (root + 1) % size;
    if (rank == root)
      for (i = 0; i < j * size; i++)
        out[i] = i;

    MPI_Scatter(out, j, MPI_INT, in, j, MPI_INT, root, MPI_COMM_WORLD);

    for (k = 0; k < j; k++) {
      if (in[k] != k + rank * j) {
        fprintf(stderr,
                "task %d bad answer (%d) at index %d k of %d (should be %d)",
                rank, in[k], k, j, (k + rank * j));
        return (0);
      }
    }
  }
  free(out);
  free(in);
  MPI_Barrier(MPI_COMM_WORLD);
  return (success);
}

/**
 * small test: the root sends a single distinct double to other processes
 **/
static int small_test(int rank, int size)
{
  int success = 1;
  int retval;
  int sendcount = 1;            // one double to each process
  int recvcount = 1;
  int i;
  double *sndbuf = NULL;
  double rcvd;
  int root = 0;                 // arbitrary choice 

  // on root, initialize sendbuf
  if (root == rank) {
    sndbuf = malloc(size * sizeof(double));
    for (i = 0; i < size; i++) {
      sndbuf[i] = (double) i;
    }
  }

  retval =
      MPI_Scatter(sndbuf, sendcount, MPI_DOUBLE, &rcvd, recvcount,
                  MPI_DOUBLE, root, MPI_COMM_WORLD);
  if (retval != MPI_SUCCESS) {
    fprintf(stderr, "(%s:%d) MPI_Scatter() returned retval=%d\n", __FILE__,
            __LINE__, retval);
    return 0;
  }
  // verification
  if ((double) rank != rcvd) {
    fprintf(stderr, "[%d] has %lf instead of %d\n", rank, rcvd, rank);
    success = 0;
  }
  return (success);
}



int main(int argc, char **argv)
{
  int size, rank;


  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* test 1 */
  if (0 == rank)
    printf("** Small Test Result: ... \n");
  if (!small_test(rank, size))
    printf("\t[%d] failed.\n", rank);
  else
    printf("\t[%d] ok.\n", rank);


  MPI_Barrier(MPI_COMM_WORLD);

  /* test 2 */
  if (0 == rank)
    printf("** IBM Test Result: ... \n");
  if (!ibm_test(rank, size))
    printf("\t[%d] failed.\n", rank);
  else
    printf("\t[%d] ok.\n", rank);

  MPI_Finalize();
  return 0;
}
