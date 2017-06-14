/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Thu Jan 3 2002 */
/* no-error-probe.c -- do some MPI calls without any errors */


#include <stdio.h>
#include "mpi.h"


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char processor_name[128];
  int namelen = 128;
  int i, j;
  double x;
  MPI_Status status;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 2)
    {
      printf ("not enough tasks\n");
    }
  else if (rank == 0)
    {
      i = 0;

      MPI_Probe (1, 0, MPI_COMM_WORLD, &status);

      MPI_Send (&i, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

      MPI_Recv (&x, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
    }
  else if (rank == 1)
    {
      x = 1.0;

      MPI_Probe (0, 0, MPI_COMM_WORLD, &status);

      MPI_Recv (&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

      MPI_Send (&x, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
