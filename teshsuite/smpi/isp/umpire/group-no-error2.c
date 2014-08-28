/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* group-no-error.c -- "correctly" construct many groups and free them */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/group-no-error2.c,v 1.1 2003/01/13 18:31:48 bronis Exp $";
#endif


/* NOTE: Some value of ITERATIONS will imply resource exhaustion */
/*       either in Umpire or MPI no matter how things are implemented */
/*       the best we can hope for is to fail gracefully... */
/* UNKNOWN N breaks umpire due to running out of memory as of 12/20/02... */
/* FAILURE IS NOT GRACEFUL AS OF THIS TIME... */
#define ITERATIONS               100


#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int i;
  char processor_name[128];
  int namelen = 128;
  MPI_Group newgroup[ITERATIONS];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  for (i = 0; i < ITERATIONS; i++) {
    MPI_Comm_group (MPI_COMM_WORLD, &newgroup[i]);
  }

  for (i = 0; i < ITERATIONS; i++) {
    MPI_Group_free (&newgroup[i]);
  }

  MPI_Barrier (MPI_COMM_WORLD);
  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
