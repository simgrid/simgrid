/* -*- C -*-

   @PROLOGUE@

   -----

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   31 Oct 2000

   hello.c -- simple hello world app

 */
#include <stdio.h>
#ifndef lint
static char *rcsid = "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/hello.c,v 1.2 2000/12/04 19:09:46 bronis Exp $";
#endif

#include "mpi.h"

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int recvbuf = 0;

  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  printf ("MPI comm size is %d with rank %d executing\n", nprocs, rank);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Reduce (&rank, &recvbuf, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0)
    {
      printf ("Reduce max is %d\n", recvbuf);
    }
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}


/* eof */
