/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive.c -- use all group constructors correctly */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/intercomm_create-no-error.c,v 1.1 2002/07/30 21:34:43 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define INTERCOMM_CREATE_TAG 666


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char processor_name[128];
  int namelen = 128;
  int i;
  MPI_Comm temp, intercomm = MPI_COMM_NULL;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 2) {
      printf ("requires at least 2 tasks\n");
  }
  else {
    /* need lots of stuff for this constructor... */
    MPI_Comm_split (MPI_COMM_WORLD, rank % 2, nprocs - rank, &temp);

    MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			  nprocs -
			  ((rank % 2) ? 2 - (nprocs % 2) : 1 + (nprocs % 2)),
			  INTERCOMM_CREATE_TAG, &intercomm);

    MPI_Comm_free (&intercomm);

    MPI_Comm_free (&temp);
  }

  MPI_Barrier (MPI_COMM_WORLD);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
