/* -*- Mode: C; -*- */
/* Modifier: Bronis R. de Supinski (bronis@llnl.gov) Wed Nov 29 2000 */
/* lost-request2.c -- create requests that are never completed */
/* Derived directly from comm-split.c by John Gyllenhaal (see below) */
/* Written by John Gyllenhaal 10/25/02 to reproduce Gary Kerbel
  * request leak that caused:
  *   0:ERROR: 0032-160 Too many communicators  (2046) in
  *            MPI_Comm_split, task 0
  * when run for too many cycles.
  *
  * Compile with:
  *   mpxlc comm_split.c -o comm_split
  * Run with a multiple of two tasks, even tasks have the problem:
  *  comm_split -nodes 1 -procs 4
  */


#include "mpi.h"
#include <stdio.h>


#define CYCLE_COUNT 5

int main
(int argc, char **argv)
{
  int  numtasks, rank, rc, split_rank, recv_int;
  int cycle;
  char processor_name[128];
  int namelen = 128;
  MPI_Comm split_comm;
  MPI_Status status;
  MPI_Request request;
  int done;

  rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS)
    {
      printf ("Error starting MPI program. Terminating.\n");
      MPI_Abort(MPI_COMM_WORLD, rc);
    }

  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  /* Must be multiple of two for this test */
  if ((numtasks & 1) != 0)
    {
      printf ("Tasks must be multiple of two for this test. Terminating.\n");
      MPI_Abort(MPI_COMM_WORLD, rc);
    }

  MPI_Barrier (MPI_COMM_WORLD);

  /* CYCLE_COUNT = 2500 causes IBM implementation to die from request leak */
  /* Can see problem with only a few (5) cycles */
  for (cycle = 0; cycle < CYCLE_COUNT; cycle ++)
    {
      /* Split adjacent pairs into their own communicator group */
      rc = MPI_Comm_split (MPI_COMM_WORLD, rank/2, rank, &split_comm);
      if (rc != MPI_SUCCESS)
	{
	  printf ("Error (rc %i) cycle %i doing MPI_Comm_split!\n",
		  rc, cycle);
	  MPI_Abort(MPI_COMM_WORLD, rc);
	}

      if (rank < 2)
	printf ("Split_comm handle %i in cycle %i\n", split_comm, cycle);

      MPI_Comm_rank(split_comm, &split_rank);

      if (split_rank == 0)
	{
	  rc = MPI_Issend (&cycle, 1, MPI_INT, 1, 0, split_comm, &request);
	  if (rc != MPI_SUCCESS)
            {
	      printf ("Error (rc %i) cycle %i doing MPI_Isend!\n",
		      rc, cycle);
	      MPI_Abort(MPI_COMM_WORLD, rc);
            }
	  /* HERE IS THE PROBLEM! Request not waited on, memory leak! */
	}
      else if (split_rank == 1)
	{
	  rc = MPI_Irecv (&recv_int, 1, MPI_INT, 0, 0, split_comm,
			  &request);
	  if (rc != MPI_SUCCESS)
	    {
	      printf ("Error (rc %i) cycle %i doing MPI_Irecv!\n",
		      rc, cycle);
	      MPI_Abort(MPI_COMM_WORLD, rc);
	    }
	  done = 0;
	  while (!done)
	    {
	      rc = MPI_Test (&request, &done, &status);
	      if (rc != MPI_SUCCESS)
		{
		  printf ("Error (rc %i) cycle %i doing MPI_Test!\n",
			  rc, cycle);
		  MPI_Abort(MPI_COMM_WORLD, rc);
		}
	    }
	  if (rank == 1)
	    {
	      printf ("Received %i in recv_int\n", recv_int);
	    }
	}

      /* Free the communicator */
      rc = MPI_Comm_free (&split_comm);
      if (rc != MPI_SUCCESS)
	{
	  printf ("Error (rc %i) cycle %i doing MPI_Comm_free!\n",
		  rc, cycle);
	  MPI_Abort(MPI_COMM_WORLD, rc);
	}
    }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize();
  printf ("(%d) Finished normally\n", rank);

  return 0;
}

