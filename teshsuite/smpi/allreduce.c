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

  int root = 0, i, j, k;
  int out[MAXLEN];
  int in[MAXLEN];

  for (j = 1; j <= MAXLEN; j *= 10) {
    for (i = 0; i < j; i++)
      out[i] = i;

    MPI_Allreduce(out, in, j, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == root) {
      for (k = 0; k < j; k++) {
        if (in[k] != k * size) {
          printf("bad answer (%d) at index %d of %d (should be %d)", in[k],
                 k, j, k * size);
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


  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  if (0 == rank)
    printf("** IBM Test Result: ... \n");
  if (!ibm_test(rank, size))
    printf("\t[%d] failed.\n", rank);
  else
    printf("\t[%d] ok.\n", rank);

  MPI_Finalize();
  return 0;
}
