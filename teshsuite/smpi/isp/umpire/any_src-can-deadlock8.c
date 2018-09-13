/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Tue Aug 26 2003 */
/* any_src-can-deadlock8.c -- deadlock occurs if task 0 receives */
/*                            from task 2 first; sleeps generally */
/*                            make order 1 before 2 with all task */
/*                            0 ops being posted after both 1 and 2 */
/*                            same as any_src-can-deadlock6.c */
/*                            except tasks 1 and 2 are interchanged */

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
  MPI_Request req;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 3)
    {
      printf ("not enough tasks\n");
    }
  else if (rank == 0)
    {
//      sleep (60);

      MPI_Irecv (buf0, buf_size, MPI_INT,
		 MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &req);

      MPI_Recv (buf1, buf_size, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

      MPI_Send (buf1, buf_size, MPI_INT, 2, 0, MPI_COMM_WORLD);

      MPI_Recv (buf1, buf_size, MPI_INT,
		MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

      MPI_Wait (&req, &status);
    }
  else if (rank == 2)
    {
      memset (buf0, 0, buf_size*sizeof(int));

  //    sleep (30);

      MPI_Send (buf0, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);

      MPI_Recv (buf1, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

      MPI_Send (buf1, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
  else if (rank == 1)
    {
      memset (buf1, 1, buf_size*sizeof(int));

      MPI_Send (buf1, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
