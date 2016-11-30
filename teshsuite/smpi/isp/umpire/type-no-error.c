/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* type-no-error.c -- construct a type and free it */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-error.c,v 1.1 2002/05/29 16:09:51 bronis Exp $";
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
  MPI_Datatype newtype, newtype2;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);
  MPI_Type_contiguous (128, MPI_INT, &newtype);
  MPI_Type_free (&newtype);
  MPI_Barrier (comm);
  /* now with an alias... */
  MPI_Type_contiguous (128, MPI_INT, &newtype);
  newtype2 = newtype;
  MPI_Type_free (&newtype2);
  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
