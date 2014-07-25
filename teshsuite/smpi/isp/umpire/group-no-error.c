/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* group-no-error.c -- construct a group and free it */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/group-no-error.c,v 1.2 2003/01/13 18:31:48 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char processor_name[128];
  int namelen = 128;
  MPI_Group newgroup, newgroup2;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Comm_group (MPI_COMM_WORLD, &newgroup);
  MPI_Group_free (&newgroup);
  MPI_Barrier (MPI_COMM_WORLD);
  /* now with an alias... */
  MPI_Comm_group (MPI_COMM_WORLD, &newgroup);
  newgroup2 = newgroup;
  MPI_Group_free (&newgroup2);
  MPI_Barrier (MPI_COMM_WORLD);
  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
