/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Fri Mar  17 2000 */
/* no-error.c -- do some MPI calls without any errors */

#include <stdio.h>
#include "mpi.h"

#define buf_size 128

#define GRAPH_SZ 4

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
  MPI_Comm comm;
  int drank, dnprocs;
  int graph_index[] = { 2, 3, 4, 6 };
  int graph_edges[] = { 1, 3, 0, 3, 0, 2 };

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 4) {
    printf ("not enough tasks\n");
  }
  else {
    /* create the graph on p.268 MPI: The Complete Reference... */
    MPI_Graph_create (MPI_COMM_WORLD, GRAPH_SZ,
		      graph_index, graph_edges, 1, &comm);

    if (comm != MPI_COMM_NULL) {
      MPI_Comm_size (comm, &dnprocs);
      MPI_Comm_rank (comm, &drank);

      if (dnprocs > 1) {
	if (drank == 0) {
	  memset (buf0, 0, buf_size*sizeof(int));

	  MPI_Recv (buf1, buf_size, MPI_INT, 1, 0, comm, &status);

	  MPI_Send (buf0, buf_size, MPI_INT, 1, 0, comm);
	}
	else if (drank == 1) {
	  memset (buf1, 1, buf_size*sizeof(int));

	  MPI_Recv (buf0, buf_size, MPI_INT, 0, 0, comm, &status);

	  MPI_Send (buf1, buf_size, MPI_INT, 0, 0, comm);
	}
      }
      else {
	printf ("(%d) Derived communicator too small (size = %d)\n",
		rank, dnprocs);
      }

      MPI_Comm_free (&comm);
    }
    else {
      printf ("(%d) Got MPI_COMM_NULL\n", rank);
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
