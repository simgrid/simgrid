/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Thu Nov 30 2000 */
/* no-error3.c -- do some MPI calls without any errors */

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
  MPI_Request aReq[2];
  MPI_Status aStatus[2];

  MPI_Status status;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 2) {
      printf ("not enough tasks\n");
  }
  else {
    if (rank == 0) {
      memset (buf0, 0, buf_size*sizeof(int));

      MPI_Send_init (buf0, buf_size, MPI_INT, 1, 0, MPI_COMM_WORLD, &aReq[0]);
      MPI_Recv_init (buf1, buf_size, MPI_INT,
		     MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &aReq[1]);

      MPI_Start (&aReq[0]);
      MPI_Start (&aReq[1]);

      MPI_Waitall (2, aReq, aStatus);

      memset (buf0, 1, buf_size*sizeof(int));

      MPI_Startall (2, aReq);
      MPI_Waitall (2, aReq, aStatus);
    }
    else if (rank == 1) {
      memset (buf1, 1, buf_size*sizeof(int));

      MPI_Recv_init (buf0, buf_size, MPI_INT,
		     MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &aReq[0]);
      MPI_Send_init (buf1, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &aReq[1]);

      MPI_Start (&aReq[0]);
      MPI_Start (&aReq[1]);

      MPI_Waitall (2, aReq, aStatus);

      memset (buf1, 0, buf_size*sizeof(int));

      MPI_Startall (2, aReq);
      MPI_Waitall (2, aReq, aStatus);
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Request_free (&aReq[0]);
  MPI_Request_free (&aReq[1]);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
