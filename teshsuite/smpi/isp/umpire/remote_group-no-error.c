/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive.c -- use all group constructors correctly */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/remote_group-no-error.c,v 1.1 2002/07/30 21:34:43 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define INTERCOMM_CREATE_TAG 666


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int i;
  MPI_Group newgroup;
  MPI_Group newgroup2;
  MPI_Comm temp;
  MPI_Comm intercomm = MPI_COMM_NULL;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);

  if (nprocs < 3) {
      printf ("requires at least 3 tasks\n");
  }
  else {
    /* create the groups */
    /* need lots of stuff for this constructor... */
    MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &temp);

    if (rank % 3) {
      MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			    (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			    nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			    INTERCOMM_CREATE_TAG, &intercomm);

      MPI_Comm_remote_group (intercomm, &newgroup);

      MPI_Comm_free (&intercomm);
    }
    else {
      MPI_Comm_group (temp, &newgroup);
    }

    MPI_Comm_free (&temp);

    MPI_Group_free (&newgroup);

    MPI_Barrier (comm);

    /* create the groups again and free with an alias... */
    /* need lots of stuff for this constructor... */
    MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &temp);

    if (rank % 3) {
      MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			    (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			    nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			    INTERCOMM_CREATE_TAG, &intercomm);

      MPI_Comm_remote_group (intercomm, &newgroup);

      MPI_Comm_free (&intercomm);
    }
    else {
      MPI_Comm_group (temp, &newgroup);
    }

    MPI_Comm_free (&temp);

    newgroup2 = newgroup;
    MPI_Group_free (&newgroup2);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
