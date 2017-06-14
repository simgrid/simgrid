/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Wed Oct 30 2002 */
/* no-error-probe-any_src.c -- do some MPI calls without any errors */
/* adapted from MPI The Complete Reference, p. 77... */


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

  /* use probe to guarantee msg has arrived when blocking recv is called */
  if (nprocs < 3)
    {
      printf ("not enough tasks\n");
    }
  else if (rank == 0)
    {
      i = 0;

      MPI_Send (&i, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
    }
  else if (rank == 1)
    {
      x = 1.0;

      MPI_Send (&x, 1, MPI_DOUBLE, 2, 0, MPI_COMM_WORLD);
    }
  else if (rank == 2)
    {
      for (j = 0; j < 2; j++) {
	MPI_Probe (MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

	if (status.MPI_SOURCE == 0)
	  MPI_Recv (&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
	else
	  MPI_Recv (&x, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
      }
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
