/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (vetter3@llnl.gov) Thu Feb 24 2000 */

/* type-no-free.c -- do a type commit and no free */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-free.c,v 1.2 2000/12/04 19:09:46 bronis Exp $";
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
  MPI_Datatype newtype;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);
  MPI_Type_contiguous (128, MPI_INT, &newtype);
  MPI_Type_commit (&newtype);
  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
