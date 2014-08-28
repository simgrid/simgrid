/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* group-no-free3.c -- construct many groups without freeing some */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/group-no-free3.c,v 1.1 2003/01/13 18:31:48 bronis Exp $";
#endif


/* NOTE: Some value of ITERATIONS will imply resource exhaustion */
/*       either in Umpire or MPI no matter how things are implemented */
/*       the best we can hope for is to fail gracefully... */
/* UNKNOWN N breaks umpire due to running out of memory as of 12/20/02... */
/* FAILURE IS NOT GRACEFUL AS OF THIS TIME... */
#define ITERATIONS                   10
#define GROUPS_PER_ITERATION          3
#define GROUPS_LOST_PER_ITERATION     1


#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int i, j;
  char processor_name[128];
  int namelen = 128;
  MPI_Group worldgroup, newgroup[GROUPS_PER_ITERATION];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Comm_group (MPI_COMM_WORLD, &worldgroup);

  MPI_Barrier (MPI_COMM_WORLD);

  for (i = 0; i < ITERATIONS; i++) {
    for (j = 0; j < GROUPS_PER_ITERATION; j++) {
      /* create groups that don't include the local rank... */
      MPI_Group_excl (worldgroup, 1, &rank, &newgroup[j]);

      if (j < GROUPS_PER_ITERATION - GROUPS_LOST_PER_ITERATION) {
	MPI_Group_free (&newgroup[j]);
      }
    }
  }

  MPI_Group_free (&worldgroup);
  MPI_Barrier (MPI_COMM_WORLD);
  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
