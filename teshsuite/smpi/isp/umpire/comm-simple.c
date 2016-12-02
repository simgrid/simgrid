/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (vetter3@llnl.gov) Thu Feb 24 2000 */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/comm-simple.c,v 1.2 2000/12/04 19:09:45 bronis Exp $";
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

  printf ("WARNING: doesn't really deadlock yet! work-in-progress.\n");

  MPI_Barrier (comm);

  {
    int color = rank % 2;
    int key = 1;
    int nrank;
    int nsize;
    int dat = 0;

    MPI_Comm_split (comm, color, key, &newcomm);

    MPI_Comm_size (newcomm, &nsize);
    MPI_Comm_rank (newcomm, &nrank);
    printf ("world task %p/%d/%d maps to new comm task %p/%d/%d\n",
	    comm, nprocs, rank, newcomm, nsize, nrank);

    if (nrank == 0)
      {
	dat = 1000 + color;
      }

    MPI_Bcast (&dat, 1, MPI_INT, 0, newcomm);

    printf ("world task %p/%d/%d maps to new comm task %p/%d/%d --> %d\n",
	    comm, nprocs, rank, newcomm, nsize, nrank, dat);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
