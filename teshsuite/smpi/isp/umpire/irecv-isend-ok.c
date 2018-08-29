/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Fri Mar  17 2000 */
/* no-error.c -- do some MPI calls without any errors */

#include <stdio.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char processor_name[128];
  int namelen = 128;
  int buf0[buf_size];
  int buf1[buf_size];
  MPI_Status status;
  MPI_Request req1;
  MPI_Request req2;

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
  else
    {
      int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
      int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
      memset (buf0, rank, buf_size*sizeof(int));
      memset (buf1, rank, buf_size*sizeof(int));
      MPI_Irecv (buf0, buf_size, MPI_INT, src, 0, MPI_COMM_WORLD, &req1);
      MPI_Isend (buf1, buf_size, MPI_INT, dest, 0, MPI_COMM_WORLD, &req2);
      MPI_Wait(&req2,&status);
      MPI_Wait(&req1,&status);
    }

  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
