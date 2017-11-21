/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (vetter3@llnl.gov) Thu Feb 24 2000 */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/comm-translate-ranks.c,v 1.1.1.1 2000/08/23 17:28:26 vetter Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  MPI_Comm newcomm;
  int key = -1;
  int nrank;
  int nsize;
  int color = -1;


  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);

  {
    color = rank % 2;
    key = 1;
    MPI_Comm_split (comm, color, key, &newcomm);

    MPI_Comm_size (newcomm, &nsize);
    MPI_Comm_rank (newcomm, &nrank);
    printf ("world task %p/%d/%d maps to new comm task %p/%d/%d\n",
	    comm, nprocs, rank, newcomm, nsize, nrank);
  }

  MPI_Barrier (comm);

  /* after every comm constructor, fetch the rank translation from the
     0 rank of that comm (assume that there is a 0). */

  if (nrank == 0)
    {
      int i;
      MPI_Group wcGroup;
      int wcRanks[128];
      MPI_Group ncGroup;
      int ncRanks[128];

      MPI_Comm_group (comm, &wcGroup);
      MPI_Comm_group (newcomm, &ncGroup);

      for (i = 0; i < nprocs; i++)
	{
	  wcRanks[i] = i;
	}

      MPI_Group_translate_ranks (wcGroup, nprocs, wcRanks, ncGroup, ncRanks);

      for (i = 0; i < nprocs; i++)
	{
	  if (ncRanks[i] == MPI_UNDEFINED)
	    {
	      printf ("World rank %d ->\tUNDEFINED\n", wcRanks[i]);
	    }
	  else
	    {
	      printf ("World rank %d ->\t%d\n", wcRanks[i], ncRanks[i]);
	    }
	}
    }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
