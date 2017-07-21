/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */
/* no-error-interleaved-isend.c - use isends with vector type */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/no-error-interleaved-isend.c,v 1.2 2002/06/07 20:41:22 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
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
  int buf[buf_size];
  int i;
  MPI_Request req;
  MPI_Status status;
  MPI_Datatype strided_type;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Type_vector (buf_size/2, 1, 2, MPI_INT, &strided_type);
  MPI_Type_commit (&strided_type);

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank < 2)
    {
      for (i = rank; i < buf_size; i = i + 2)
	buf[i] = i;

      MPI_Isend (&buf[rank], 1,
		 strided_type, (rank + 1) % 2, 0, comm, &req);

      MPI_Recv (&buf[(rank + 1) % 2], 1,
		strided_type, (rank + 1) % 2, 0, comm, &status);

      MPI_Wait (&req, &status);

      for (i = 0; i < buf_size; i++)
	assert (buf[i] == i);
    }

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Type_free (&strided_type);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
