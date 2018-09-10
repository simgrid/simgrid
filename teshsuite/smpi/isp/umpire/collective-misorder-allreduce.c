/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (j-vetter@llnl.gov) Mon Nov  1 1999 */
/* collective-misorder.c -- do some collective operations (w/ one of them out of order) */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/collective-misorder-allreduce.c,v 1.2 2000/12/04 19:09:45 bronis Exp $";
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
  int sbuf[buf_size];
  int rbuf[buf_size];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  memset (sbuf, 0, buf_size*sizeof(int));
  memset (rbuf, 1, buf_size*sizeof(int));

  MPI_Barrier (comm);

  switch (rank)
    {
    case 0:
      MPI_Reduce(sbuf,rbuf,1,MPI_INT,MPI_MAX,0,comm);
      break;

    default:
      MPI_Allreduce(sbuf,rbuf,1,MPI_INT, MPI_MAX, comm);
      break;
    }

  MPI_Barrier(comm);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
