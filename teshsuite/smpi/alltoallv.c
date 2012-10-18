/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

/*
   This program tests MPI_Alltoallv by having processor i send different
   amounts of data to each processor.

   Because there are separate send and receive types to alltoallv,
   there need to be tests to rearrange data on the fly.  Not done yet.

   The first test sends i items to processor i from all processors.

   Currently, the test uses only MPI_INT; this is adequate for testing systems
   that use point-to-point operations
 */


/* example values:
 * For 3 processes:
 * <0> sbuf: (#9):   [0][1][2][3][4][5][6][7][8]
   <0> scount: (#3): [0][1][2]
   <0> rcount: (#3): [0][0][0]
   <0> sdisp: (#3):  [0][1][3]
   <0> rdisp: (#3):  [0][0][0]

   <1> sbuf: (#9):   [100][101][102][103][104][105][106][107][108]
   <1> scount: (#3): [0][1][2]
   <1> rcount: (#3): [1][1][1]
   <1> sdisp: (#3):  [0][1][3]
   <1> rdisp: (#3):  [0][1][2]

   <2> sbuf: (#9):   [200][201][202][203][204][205][206][207][208]
   <2> scount: (#3): [0][1][2]
   <2> rcount: (#3): [2][2][2]
   <2> sdisp: (#3):  [0][1][3]
   <2> rdisp: (#3):  [0][2][4]

   after MPI_Alltoallv :
   <0> rbuf: (#9):   [0][-1][-2][-3][-4][-5][-6][-7][-8]
   <1> rbuf: (#9):   [1][101][201][-3][-4][-5][-6][-7][-8]
   <2> rbuf: (#9):   [3][4][103][104][203][204][-6][-7][-8]
*/


static void print_buffer_int(void *buf, int len, char *msg, int rank)
{
  int tmp, *v;
  printf("**<%d> %s (#%d): ", rank, msg, len);
  for (tmp = 0; tmp < len; tmp++) {
    v = buf;
    printf("[%d]", v[tmp]);
  }
  printf("\n");
  free(msg);
}


int main(int argc, char **argv)
{

  MPI_Comm comm;
  int *sbuf, *rbuf, *erbuf;
  int rank, size;
  int *sendcounts, *recvcounts, *rdispls, *sdispls;
  int i, j, *p, err;

  MPI_Init(&argc, &argv);
  err = 0;

  comm = MPI_COMM_WORLD;

  /* Create the buffer */
  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);
  sbuf = (int *) malloc(size * size * sizeof(int));
  rbuf = (int *) malloc(size * size * sizeof(int));
  erbuf = (int *) malloc(size * size * sizeof(int));    // expected
  if (!sbuf || !rbuf) {
    fprintf(stderr, "Could not allocated buffers!\n");
    MPI_Abort(comm, 1);
  }

  /* Load up the buffers */
  for (i = 0; i < size * size; i++) {
    sbuf[i] = i + 100 * rank;
    rbuf[i] = -i;
    erbuf[i] = -i;
  }

  /* Create and load the arguments to alltoallv */
  sendcounts = (int *) malloc(size * sizeof(int));
  recvcounts = (int *) malloc(size * sizeof(int));
  rdispls = (int *) malloc(size * sizeof(int));
  sdispls = (int *) malloc(size * sizeof(int));
  if (!sendcounts || !recvcounts || !rdispls || !sdispls) {
    fprintf(stderr, "Could not allocate arg items!\n");
    MPI_Abort(comm, 1);
  }
  for (i = 0; i < size; i++) {
    sendcounts[i] = i;
    recvcounts[i] = rank;
    rdispls[i] = i * rank;
    sdispls[i] = (i * (i + 1)) / 2;
  }

  /* debug */
  /* 
     print_buffer_int( sbuf, size*size, strdup("sbuf:"),rank);
     print_buffer_int( sendcounts, size, strdup("scount:"),rank);
     print_buffer_int( recvcounts, size, strdup("rcount:"),rank);
     print_buffer_int( sdispls, size, strdup("sdisp:"),rank);
     print_buffer_int( rdispls, size, strdup("rdisp:"),rank);
   */


  /* debug : erbuf */
  /* debug
     for (i=0; i<size; i++) {
     for (j=0; j<rank; j++) {
     *(erbuf+j+ rdispls[i]) = i * 100 + (rank*(rank+1))/2 + j; 
     }
     }
   */


  //print_buffer_int( erbuf, size*size, strdup("erbuf:"),rank);

  MPI_Alltoallv(sbuf, sendcounts, sdispls, MPI_INT,
                rbuf, recvcounts, rdispls, MPI_INT, comm);

  // debug: print_buffer_int( rbuf, size*size, strdup("rbuf:"),rank);


  /* Check rbuf */
  for (i = 0; i < size; i++) {
    p = rbuf + rdispls[i];
    for (j = 0; j < rank; j++) {
      if (p[j] != i * 100 + (rank * (rank + 1)) / 2 + j) {
        fprintf(stderr, "** Error: <%d> got %d expected %d for %dth\n",
                rank, p[j], (i * (i + 1)) / 2 + j, j);
        err++;
      }
    }
  }

  /* Summary */
  if (err > 0) {
    printf("<%d> Alltoallv test: failure (%d errors).\n", rank, err);
  }
  if (0 == rank) {
    printf("* Alltoallv TEST COMPLETE.\n");
  }
  free(sdispls);
  free(rdispls);
  free(recvcounts);
  free(sendcounts);
  free(rbuf);
  free(sbuf);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
