/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Tue Dec 10 2002 */

/* type-no-free3.c -- create many types, failing to free some */
/*                    much like type-no-free2.c but use types */
/*                    to communicate and thus exercise umpire more */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-free3.c,v 1.2 2003/01/13 18:31:49 bronis Exp $";
#endif

/* NOTE: Some value of ITERATIONS will imply resource exhaustion */
/*       either in Umpire or MPI no matter how things are implemented */
/*       the best we can hope for is to fail gracefully... */
/* 10000 breaks umpire due to running out of memory as of 12/12/02... */
/* FAILURE IS NOT GRACEFUL AS OF THIS TIME... */
#define ITERATIONS                  10
#define TYPES_PER_ITERATION          3
#define TYPES_LOST_PER_ITERATION     1
#define TYPES_TO_COMMIT              1


#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int i, j, partner, buf_j;
  int buf[buf_size * TYPES_TO_COMMIT];
  char processor_name[128];
  int namelen = 128;
  MPI_Datatype newtype[TYPES_PER_ITERATION];
  MPI_Request reqs[TYPES_TO_COMMIT];
  MPI_Status statuses[TYPES_TO_COMMIT];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  if (nprocs < 2) {
    printf ("Not enough tasks\n");
    MPI_Finalize();

    return 0;
  }

  MPI_Barrier (MPI_COMM_WORLD);

  if ((nprocs % 2 == 0) ||
      (rank != nprocs - 1)) {
    partner = (rank % 2) ? rank - 1 : rank + 1;

    for (i = 0; i < ITERATIONS; i++) {
      for (j = 0; j < TYPES_PER_ITERATION; j++) {
	MPI_Type_contiguous (buf_size, MPI_INT, &newtype[j]);

	buf_j = j + TYPES_TO_COMMIT - TYPES_PER_ITERATION;

	if (buf_j >= 0) {
	  MPI_Type_commit (&newtype[j]);

	  if (rank % 2) {
	    MPI_Irecv (&buf[buf_j*buf_size], 1, newtype[j],
		       partner, buf_j, MPI_COMM_WORLD, &reqs[buf_j]);
	  }
	  else {
	    MPI_Isend (&buf[buf_j*buf_size], 1, newtype[j],
		       partner, buf_j, MPI_COMM_WORLD, &reqs[buf_j]);
	  }
	}

	if (j < TYPES_PER_ITERATION - TYPES_LOST_PER_ITERATION) {
	  MPI_Type_free (&newtype[j]);
	}
      }

      MPI_Waitall (TYPES_TO_COMMIT, reqs, statuses);
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();

  return 0;
}

/* EOF */
