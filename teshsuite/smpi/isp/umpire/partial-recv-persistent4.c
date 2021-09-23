/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Fri Jan 24, 2003 */
/* partial-recv-persistent2.c -- do persistent ops w/oversized recv bufs */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/partial-recv-persistent4.c,v 1.1 2003/05/17 11:05:31 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define BUF_SIZE 128
#define SLOP 128


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int buf[BUF_SIZE * 2 + SLOP];
  int j, k, flag;
  MPI_Request aReq[2];
  MPI_Status aStatus[2];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  if (rank == 0) {
    /* set up persistent sends... */
    MPI_Send_init (buf, BUF_SIZE, MPI_INT, 1, 0, comm, &aReq[0]);
    MPI_Send_init (&buf[BUF_SIZE], BUF_SIZE, MPI_INT, 1, 1, comm, &aReq[1]);

    /* initialize the send buffers */
    for (k = 0; k < BUF_SIZE; k++) {
      buf[k] = k;
      buf[BUF_SIZE + k] = BUF_SIZE - 1 - k;
    }
  }
  else {
    /* set up the persistent receives... */
    MPI_Recv_init (buf, BUF_SIZE, MPI_INT, 0, 0, comm, &aReq[0]);
    MPI_Recv_init (&buf[BUF_SIZE],BUF_SIZE+SLOP,MPI_INT,0,1,comm,&aReq[1]);
  }

  for (k = 0; k < 4; k++) {
    if (rank == 1) {
      /* zero out all of the receive buffers */
      bzero (buf, sizeof(int) * BUF_SIZE * 2 + SLOP);
    }

    /* start the persistent requests... */
    if (rank < 2) {
      if (k % 2) {
	MPI_Startall (2, aReq);
      }
      else {
	MPI_Start (&aReq[0]);
	MPI_Start (&aReq[1]);
      }

      MPI_Barrier(MPI_COMM_WORLD);

      /* complete the requests */
      if (k/2) {
	/* use MPI_Wait */
	for (j = 0; j < 2; j++) {
	  MPI_Wait (&aReq[j], &aStatus[j]);
	}
      }
      else {
	/* use MPI_Test */
	for (j = 0; j < 2; j++) {
	  flag = 0;

	  while (!flag) {
	    MPI_Test (&aReq[j], &flag, &aStatus[j]);
	  }
	}
      }
    }
    else {
      /* Barrier to ensure receives are posted for rsends... */
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank < 2) {
    /* free the persistent requests */
    MPI_Request_free (&aReq[0]);
    MPI_Request_free (&aReq[1]);
  }

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
