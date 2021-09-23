/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive-with-isends.c -- send with weird types */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/partial-recv.c,v 1.1 2002/10/24 17:04:56 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"


typedef struct _test_small_struct_t
{
  double the_double;
  char the_char;
}
test_small_struct_t;


typedef struct _test_big_struct_t
{
  double the_double;
  char the_char;
  double the_other_double;
}
test_big_struct_t;


#define SMALL_SIZE  10
#define BIG_SIZE    10

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int i;
  MPI_Aint basic_extent;
  int blocklens[3];
  MPI_Aint displs[3];
  MPI_Datatype structtypes[3];
  MPI_Datatype newtype[2];
  MPI_Request aReq[2];
  MPI_Status aStatus[2];
  test_small_struct_t small_struct_buf[SMALL_SIZE];
  test_big_struct_t big_struct_buf[BIG_SIZE];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  structtypes[0] = MPI_DOUBLE;
  structtypes[1] = MPI_CHAR;
  structtypes[2] = MPI_DOUBLE;
  blocklens[0] = blocklens[1] = blocklens[2] = 1;
  displs[0] = 0;
  displs[1] = sizeof(double);
  displs[2] =
    ((void *) &(big_struct_buf[0].the_other_double)) -
    ((void *) big_struct_buf);

  if (displs[2] < 0) displs[2] = -displs[2];

  MPI_Barrier (comm);

  /* create the types */
  MPI_Type_struct (2, blocklens, displs, structtypes, &newtype[0]);
  MPI_Type_struct (3, blocklens, displs, structtypes, &newtype[1]);

  MPI_Type_extent (newtype[0], &basic_extent);
  if (basic_extent != sizeof (test_small_struct_t)) {
    fprintf (stderr, "(%d): Unexpected extent for small struct\n", rank);
    MPI_Abort (MPI_COMM_WORLD, 666);
  }

  MPI_Type_extent (newtype[1], &basic_extent);
  if (basic_extent != sizeof (test_big_struct_t)) {
    fprintf (stderr, "(%d): Unexpected extent for big struct\n", rank);
    MPI_Abort (MPI_COMM_WORLD, 666);
  }

  MPI_Type_commit (&newtype[0]);
  MPI_Type_commit (&newtype[1]);

  if (rank == 0) {
    /* initialize buffers */
    for (i = 0; i < SMALL_SIZE; i++) {
      small_struct_buf[i].the_double = 1.0;
      small_struct_buf[i].the_char = 'a';
    }

    for (i = 0; i < BIG_SIZE; i++) {
      big_struct_buf[i].the_double = 1.0;
      big_struct_buf[i].the_char = 'a';
      big_struct_buf[i].the_other_double = 1.0;
    }

    /* set up the sends */
    MPI_Isend (small_struct_buf, 1, newtype[0], 1, 0, comm, &aReq[0]);
    MPI_Isend (big_struct_buf, 1, newtype[1], 1, 1, comm, &aReq[1]);
  }
  else if (rank == 1) {
    /* initialize buffers */
    for (i = 0; i < SMALL_SIZE; i++) {
      small_struct_buf[i].the_double = 2.0;
      small_struct_buf[i].the_char = 'b';
    }

    for (i = 0; i < BIG_SIZE; i++) {
      big_struct_buf[i].the_double = 2.0;
      big_struct_buf[i].the_char = 'b';
      big_struct_buf[i].the_other_double = 2.0;
    }

    /* set up the receives... */
    MPI_Irecv (big_struct_buf, BIG_SIZE, newtype[1],0,0, comm, &aReq[0]);
    MPI_Irecv (small_struct_buf, SMALL_SIZE, newtype[0],0,1, comm, &aReq[1]);
  }

  if ((rank == 0) || (rank == 1)) {
    /* wait on everything... */
    MPI_Waitall (2, aReq, aStatus);
  }

  for (i = 0; i < 2; i++) {
    MPI_Type_free (&newtype[i]);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
