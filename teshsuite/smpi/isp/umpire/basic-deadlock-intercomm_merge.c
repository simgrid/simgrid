/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Fri Mar  17 2000 */
/* no-error.c -- do some MPI calls without any errors */

#include <stdio.h>
#include "mpi.h"

#define buf_size 128

#define INTERCOMM_CREATE_TAG 666

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
  MPI_Comm comm, temp, intercomm;
  int drank, dnprocs, rleader;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 3) {
    printf ("not enough tasks\n");
  }
  else {
    /* need to make split communicator temporarily... */
    MPI_Comm_split (MPI_COMM_WORLD, rank % 2, nprocs - rank, &temp);

    if (temp != MPI_COMM_NULL) {
      /* create an intercommunicator temporarily so can merge it... */
      rleader = ((rank + nprocs) % 2) ?  nprocs - 2 : nprocs - 1;
      MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD, rleader,
			    INTERCOMM_CREATE_TAG, &intercomm);

      MPI_Comm_free (&temp);

      if (intercomm == MPI_COMM_NULL) {
	printf ("(%d) MPI_Intercomm_Create returned MPI_COMM_NULL\n", rank);
	printf ("(%d) Aborting...\n", rank);
	MPI_Abort (MPI_COMM_WORLD, 666);
	exit (666);
      }

      MPI_Intercomm_merge (intercomm, rank % 2, &comm);

      MPI_Comm_free (&intercomm);

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
    else {
      printf ("(%d) MPI_Comm_split got MPI_COMM_NULL\n", rank);
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
