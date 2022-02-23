/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Fri Dec 20 2002 */

/* comm-dup-no-error.c - "correctly" use many communicators... */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/comm-dup-no-error.c,v 1.1 2003/01/13 18:31:47 bronis Exp $";
#endif

/* NOTE: Some value of ITERATIONS will imply resource exhaustion */
/*       either in Umpire or MPI no matter how things are implemented */
/*       the best we can hope for is to fail gracefully... */
/* Approximately 4100 gets "ERROR: 0032-160 Too many communicators" */
/* with IBM's MPI (AIX 5.1.0, PSSP 3.4) as of 1/13/03... */
/* Umpire failure is graceful - comm creates are identified... */
/* UNKNOWN N breaks umpire due to running out of memory as of 1/13/03... */
/* UMPIRE FAILURE IS NOT GRACEFUL AS OF THIS TIME IN THIS CASE... */
#define ITERATIONS               3


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
  MPI_Comm newcomm[ITERATIONS];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  for (i = 0; i < ITERATIONS; i++) {
    MPI_Comm_dup (MPI_COMM_WORLD, &newcomm[i]);
  }

  for (i = 0; i < ITERATIONS; i++) {
    MPI_Barrier (newcomm[i]);
  }


  for (i = 0; i < ITERATIONS; i++) {
    MPI_Comm_free (&newcomm[i]);
  }

  MPI_Barrier (MPI_COMM_WORLD);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
