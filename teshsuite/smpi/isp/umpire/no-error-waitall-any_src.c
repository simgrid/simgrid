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
  MPI_Status statuses[2];
  MPI_Request reqs[2];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  /* the following code is "correct although its result */
  /* is non-deterministic - in task 0, either buf0 will */
  /* hold result of memset to 0 and buf1 will hold result */
  /* of memset to 1 or buf0 will hold result of memset to */
  /* 1 and buf1 will hold result of memset to 0; in task 1, */
  /* buf1 will mirror task 0 buf1 result while, in task 2, */
  /* buf0 will mirror task 0 buf0 result; thus buf0 may */
  /* be equal to buf1 in tasks 1 and 2 or it may not... */
  if (nprocs < 3)
    {
      printf ("not enough tasks\n");
    }
  else if (rank == 0)
    {
      MPI_Irecv (buf0, buf_size, MPI_INT,
		 MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &reqs[0]);

      MPI_Irecv (buf1, buf_size, MPI_INT,
		 MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &reqs[1]);

      MPI_Waitall (2, reqs, statuses);

      MPI_Send (buf1, buf_size, MPI_INT, 1, 1, MPI_COMM_WORLD);

      MPI_Send (buf0, buf_size, MPI_INT, 2, 2, MPI_COMM_WORLD);
    }
  else if (rank == 1)
    {
      memset (buf0, 0, buf_size*sizeof(int));

      MPI_Send (buf0, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);

      MPI_Recv (buf1, buf_size, MPI_INT, 0, 1, MPI_COMM_WORLD, statuses);
    }
  else if (rank == 2)
    {
      memset (buf1, 1, buf_size*sizeof(int));

      MPI_Send (buf1, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);

      MPI_Recv (buf0, buf_size, MPI_INT, 0, 2, MPI_COMM_WORLD, statuses);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
