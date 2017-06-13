/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive.c -- use all group constructors correctly */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/group-no-error-exhaustive.c,v 1.2 2002/07/30 21:34:42 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define GROUP_CONSTRUCTOR_COUNT 8
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
  int ranks[2], ranges[1][3];
  MPI_Group newgroup[GROUP_CONSTRUCTOR_COUNT];
  MPI_Group newgroup2[GROUP_CONSTRUCTOR_COUNT];
  MPI_Comm temp;
  MPI_Comm intercomm = MPI_COMM_NULL;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  ranks[0] = 0;
  ranks[1] = 1;

  ranges[0][0] = 0;
  ranges[0][1] = 2;
  ranges[0][2] = 2;

  MPI_Barrier (comm);

  if (nprocs < 3) {
      printf ("requires at least 3 tasks\n");
  }
  else {
    /* create the groups */
    if (GROUP_CONSTRUCTOR_COUNT > 0)
      MPI_Comm_group (MPI_COMM_WORLD, &newgroup[0]);

    if (GROUP_CONSTRUCTOR_COUNT > 1)
      MPI_Group_incl (newgroup[0], 2, ranks, &newgroup[1]);

    if (GROUP_CONSTRUCTOR_COUNT > 2)
      MPI_Group_excl (newgroup[0], 2, ranks, &newgroup[2]);

    if (GROUP_CONSTRUCTOR_COUNT > 3)
      MPI_Group_range_incl (newgroup[0], 1, ranges, &newgroup[3]);

    if (GROUP_CONSTRUCTOR_COUNT > 4)
      MPI_Group_range_excl (newgroup[0], 1, ranges, &newgroup[4]);

    if (GROUP_CONSTRUCTOR_COUNT > 5)
      MPI_Group_union (newgroup[1], newgroup[3], &newgroup[5]);

    if (GROUP_CONSTRUCTOR_COUNT > 6)
      MPI_Group_intersection (newgroup[5], newgroup[2], &newgroup[6]);

    if (GROUP_CONSTRUCTOR_COUNT > 7)
      MPI_Group_difference (newgroup[5], newgroup[2], &newgroup[7]);

    if (GROUP_CONSTRUCTOR_COUNT > 8) {
      /* need lots of stuff for this constructor... */
      MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &temp);

      if (rank % 3) {
	MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			      (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			      nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			      INTERCOMM_CREATE_TAG, &intercomm);

	MPI_Comm_remote_group (intercomm, &newgroup[8]);

	MPI_Comm_free (&intercomm);
      }
      else {
	MPI_Comm_group (temp, &newgroup[8]);
      }

      MPI_Comm_free (&temp);
    }

    for (i = 0; i < GROUP_CONSTRUCTOR_COUNT; i++)
      MPI_Group_free (&newgroup[i]);

    MPI_Barrier (comm);

    /* create the groups again and free with an alias... */
    if (GROUP_CONSTRUCTOR_COUNT > 0)
      MPI_Comm_group (MPI_COMM_WORLD, &newgroup[0]);

    if (GROUP_CONSTRUCTOR_COUNT > 1)
      MPI_Group_incl (newgroup[0], 2, ranks, &newgroup[1]);

    if (GROUP_CONSTRUCTOR_COUNT > 2)
      MPI_Group_excl (newgroup[0], 2, ranks, &newgroup[2]);

    if (GROUP_CONSTRUCTOR_COUNT > 3)
      MPI_Group_range_incl (newgroup[0], 1, ranges, &newgroup[3]);

    if (GROUP_CONSTRUCTOR_COUNT > 4)
      MPI_Group_range_excl (newgroup[0], 1, ranges, &newgroup[4]);

    if (GROUP_CONSTRUCTOR_COUNT > 5)
      MPI_Group_union (newgroup[1], newgroup[3], &newgroup[5]);

    if (GROUP_CONSTRUCTOR_COUNT > 6)
      MPI_Group_intersection (newgroup[5], newgroup[2], &newgroup[6]);

    if (GROUP_CONSTRUCTOR_COUNT > 7)
      MPI_Group_difference (newgroup[5], newgroup[2], &newgroup[7]);

    if (GROUP_CONSTRUCTOR_COUNT > 8) {
      /* need lots of stuff for this constructor... */
      MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &temp);

      if (rank % 3) {
	MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			      (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			      nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			      INTERCOMM_CREATE_TAG, &intercomm);

	MPI_Comm_remote_group (intercomm, &newgroup[8]);

	MPI_Comm_free (&intercomm);
      }
      else {
	MPI_Comm_group (temp, &newgroup[8]);
      }

      MPI_Comm_free (&temp);
    }

    for (i = 0; i < GROUP_CONSTRUCTOR_COUNT; i++) {
      newgroup2[i] = newgroup[i];
      MPI_Group_free (&newgroup2[i]);
    }
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
