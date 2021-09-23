/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Mon Jan 21 2003 */
/* no-error-waitany2.c -- test behavior with MPI_Waitany & MPI_REQUEST_NULL */

#include <stdio.h>
#include <assert.h>
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
  int buf2[buf_size];
  int i, flipbit, done;
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
      MPI_Request reqs[3];

      /* first just try MPI_Waitany on explicit MPI_REQUEST_NULL set... */
      reqs[0] = reqs[1] = reqs[2] = MPI_REQUEST_NULL;

      MPI_Waitany (3, reqs, &done, &status);

      assert (done < 0);

      MPI_Irecv (buf0, buf_size, MPI_INT, 1, 1, MPI_COMM_WORLD, &reqs[0]);
      MPI_Irecv (buf1, buf_size, MPI_INT, 1, 2, MPI_COMM_WORLD, &reqs[1]);
      MPI_Irecv (buf2, buf_size, MPI_INT, 1, 3, MPI_COMM_WORLD, &reqs[2]);

      for (i = 3; i >= 0; i--) {
	MPI_Send (&flipbit, 1, MPI_INT, 1, i, MPI_COMM_WORLD);

	MPI_Waitany (3, reqs, &done, &status);
printf ("Done = %d\n", done);

	if (i > 0) {
	  assert (done == (i - 1));
	}
	else {
	  assert (done < 0);
	}
      }
    }
  else if (rank == 1)
    {
      memset (buf0, 1, buf_size*sizeof(int));

      for (i = 3; i >= 0; i--) {
	MPI_Recv (&flipbit, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &status);

	if (i > 0) {
	  MPI_Send (buf0, buf_size, MPI_INT, 0, i, MPI_COMM_WORLD);
	}
      }
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
