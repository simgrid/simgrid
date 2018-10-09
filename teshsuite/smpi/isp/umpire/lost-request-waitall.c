/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (j-vetter@llnl.gov) Mon Nov  1 1999 */
/* lost-request.c -- overwrite a request and essentially lose a synch point */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/lost-request-waitall.c,v 1.1.1.1 2000/08/23 17:28:26 vetter Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mpi.h"

#define buf_size 128

static int mydelay(void) /* about 6 seconds */
{
  int i;
  int val;
  for (i = 0; i < 3000000; i++)
    {
      val = getpid ();		/* about 2.06 usecs on snow */
    }
  return val;
}

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int tag1 = 31;
  int tag2 = 32;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int buf0[buf_size];
  int buf1[buf_size];
  MPI_Request req;
  MPI_Status status;
  MPI_Request areq[10];
  MPI_Status astatus[10];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  memset (buf0, 0, buf_size*sizeof(int));
  memset (buf1, 1, buf_size*sizeof(int));

  /* 0 sends 1 two messages, but the request gets overwritten */
  switch (rank)
    {
    case 0:
      MPI_Isend (buf0, buf_size, MPI_INT, 1, tag1, comm, &(areq[0]));
      MPI_Isend (buf1, buf_size, MPI_INT, 1, tag2, comm, &(areq[1]));
      /* do some work here */
      //mydelay ();
      MPI_Waitall (2, areq, astatus);
      break;

    case 1:
      MPI_Irecv (buf0, buf_size, MPI_INT, 0, tag1, comm, &req);
      MPI_Irecv (buf1, buf_size, MPI_INT, 0, tag2, comm, &req);	/* overwrite req */
      /* do some work here and get confused */
      MPI_Wait (&req, &status);
      break;

    default:
      /* do nothing */
      break;
    }

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
