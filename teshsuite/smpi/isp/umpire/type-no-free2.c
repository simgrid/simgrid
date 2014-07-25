/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Tue Dec 10 2002 */

/* type-no-free2.c -- create many types, failing to free some */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-free2.c,v 1.2 2003/01/13 18:31:48 bronis Exp $";
#endif

/* NOTE: Some value of ITERATIONS will imply resource exhaustion */
/*       either in Umpire or MPI no matter how things are implemented */
/*       the best we can hope for is to fail gracefully... */
/* 10000 breaks umpire due to running out of memory as of 12/12/02... */
/* FAILURE IS NOT GRACEFUL AS OF THIS TIME... */
#define ITERATIONS                  10
#define TYPES_PER_ITERATION          3
#define TYPES_LOST_PER_ITERATION     1
#define TYPES_TO_COMMIT              1


#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int i, j;
  char processor_name[128];
  int namelen = 128;
  MPI_Datatype newtype[TYPES_PER_ITERATION];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  for (i = 0; i < ITERATIONS; i++) {
    for (j = 0; j < TYPES_PER_ITERATION; j++) {
      MPI_Type_contiguous (128, MPI_INT, &newtype[j]);

      if (j >= TYPES_PER_ITERATION - TYPES_TO_COMMIT) {
	MPI_Type_commit (&newtype[j]);
      }

      if (j < TYPES_PER_ITERATION - TYPES_LOST_PER_ITERATION) {
	MPI_Type_free (&newtype[j]);
      }
    }

    if (((i % (ITERATIONS/10)) == 0) && (rank == 0))
      printf ("iteration %d completed\n", i);
  }

  MPI_Barrier (MPI_COMM_WORLD);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
