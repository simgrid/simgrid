/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (vetter3@llnl.gov) Thu Feb 24 2000 */

/* type-commit-twice.c -- do a type commit twice w/ the same type */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/comm-dup-no-free.c,v 1.1.1.1 2000/08/23 17:28:26 vetter Exp $";
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
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  MPI_Comm newcomm;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);
  MPI_Comm_dup (comm, &newcomm);	/* not freed */
  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
