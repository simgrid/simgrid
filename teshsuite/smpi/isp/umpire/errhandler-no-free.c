/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* errhandler-no-error.c -- construct some MPI_Errhandlers and free them */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/errhandler-no-free.c,v 1.1 2002/05/29 16:09:48 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"

/* multiple instances of same errhandler to exercise more Umpire code... */
#define ERRHANDLER_COUNT  5



void
myErrhandler (MPI_Comm *comm, int *errorcode, ...)
{
  char      buf[MPI_MAX_ERROR_STRING];
  int       error_strlen;

  /* print alert */
  fprintf (stderr, "Caught an MPI Error! Time to abort!\n");

  /* get and print MPI error message... */
  MPI_Error_string (*(errorcode), buf, &error_strlen);
  fprintf (stderr, "%s\n", buf);

  MPI_Abort (*comm, *errorcode);

  return;
}


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  int i;
  char processor_name[128];
  int namelen = 128;
  MPI_Errhandler newerrhandler[ERRHANDLER_COUNT];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);

  for (i = 0; i < ERRHANDLER_COUNT; i++)
    MPI_Errhandler_create (myErrhandler, &newerrhandler[i]);

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
