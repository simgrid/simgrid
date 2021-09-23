

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define buf_size 128

int
main (int argc, char **argv)
{
  int rank;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int buf0[buf_size];
  int buf1[buf_size];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  if (rank == 0) {
    memset (buf0, 0, buf_size*sizeof(int));
    MPI_Bcast (buf0, buf_size, MPI_INT, 1, MPI_COMM_WORLD);
    MPI_Bcast (buf0, buf_size, MPI_INT, 0, MPI_COMM_WORLD);
  }
  else {
    if (rank == 1)
      memset (buf1, 1, buf_size*sizeof(int));

    MPI_Bcast (buf0, buf_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast (buf0, buf_size, MPI_INT, 1, MPI_COMM_WORLD);
  }

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
