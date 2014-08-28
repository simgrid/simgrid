/* $Header: /usr/gapps/asde/cvs-vault/umpire/tests/deadlock-config.c,v 1.2 2001/09/20 22:27:28 bronis Exp $ */

#include <stdio.h>
#include "mpi.h"

#define buf_size 32000

main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  float data[buf_size];
  int tag = 30;
  char processor_name[128];
  int namelen = buf_size;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  if (rank == 0)
    {
      printf ("WARNING: This test depends on the MPI's eager limit. "
	      "Set it appropriately.\n");
    }
  printf ("Initializing (%d of %d)\n", rank, nprocs);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);
  {
    int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
    data[0] = rank;
    MPI_Send (data, buf_size, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
    printf ("(%d) sent data %f\n", rank, data[0]);
    fflush (stdout);
  }
  {
    int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
    MPI_Status status;
    MPI_Recv (data, buf_size, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
    printf ("(%d) got data %f\n", rank, data[0]);
    fflush (stdout);
  }
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

				 /* EOF */
