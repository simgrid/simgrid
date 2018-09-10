/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (j-vetter@llnl.gov) Mon Nov  1 1999 */
/* collective-misorder.c -- do some collective operations (w/ one of them out of order) */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/collective-misorder.c,v 1.2 2000/12/04 19:09:45 bronis Exp $";
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
  int buf0[buf_size];
  int buf1[buf_size];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  memset (buf0, 0, buf_size*sizeof(int));
  memset (buf1, 1, buf_size*sizeof(int));

  MPI_Barrier (comm);
  MPI_Barrier (comm);

  switch (rank)
    {
    case 0:
      MPI_Bcast (buf0, buf_size, MPI_INT, 1, comm);	/* note that I didn't use root == 0 */
      MPI_Barrier (comm);
      break;

    default:
      MPI_Barrier (comm);
      MPI_Bcast (buf0, buf_size, MPI_INT, 1, comm);
      break;
    }
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
