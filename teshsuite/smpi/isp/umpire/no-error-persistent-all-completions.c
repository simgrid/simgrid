/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (j-vetter@llnl.gov) Mon Nov  1 1999 */
/* lost-request.c -- overwrite a request and essentially lose a synch point */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/no-error-persistent-all-completions.c,v 1.1 2002/01/14 18:58:06 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"

#define BUF_SIZE 128
#define NUM_COMPLETION_MECHANISMS 8

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int buf[BUF_SIZE * 2];
  int i, j, k, index, outcount, flag;
  int indices[2];
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
    MPI_Send_init (&buf[0], BUF_SIZE, MPI_INT, 1, 0, comm, &aReq[0]);
    MPI_Send_init (&buf[BUF_SIZE], BUF_SIZE, MPI_INT, 1, 1, comm, &aReq[1]);

    /* initialize the send buffers */
    for (i = 0; i < BUF_SIZE; i++) {
      buf[i] = i;
      buf[BUF_SIZE + i] = BUF_SIZE - 1 - i;
    }
  }

  for (k = 0; k < (NUM_COMPLETION_MECHANISMS * 2); k++) {
    if (rank == 1) {
      /* zero out the receive buffers */
      bzero (buf, sizeof(int) * BUF_SIZE * 2);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
      /* start the persistent sends... */
      if (k % 2) {
	MPI_Startall (2, &aReq[0]);
      }
      else {
	for (j = 0; j < 2; j++) {
	  MPI_Start (&aReq[j]);
	}
      }

      /* complete the sends */
      switch (k/2) {
      case 0:
	/* use MPI_Wait */
	for (j = 0; j < 2; j++) {
	  MPI_Wait (&aReq[j], &aStatus[j]);
	}
	break;

      case 1:
	/* use MPI_Waitall */
	MPI_Waitall (2, aReq, aStatus);
	break;

      case 2:
	/* use MPI_Waitany */
	for (j = 0; j < 2; j++) {
	  MPI_Waitany (2, aReq, &index, aStatus);
	}
	break;

      case 3:
	/* use MPI_Waitsome */
	j = 0;
	while (j < 2) {
	  MPI_Waitsome (2, aReq, &outcount, indices, aStatus);
	  j += outcount;
	}
	break;

      case 4:
	/* use MPI_Test */
	for (j = 0; j < 2; j++) {
	  flag = 0;
	  while (!flag) {
	    MPI_Test (&aReq[j], &flag, &aStatus[j]);
	  }
	}
	break;

      case 5:
	/* use MPI_Testall */
	flag = 0;
	while (!flag) {
	  MPI_Testall (2, aReq, &flag, aStatus);
	}
	break;

      case 6:
	/* use MPI_Testany */
	for (j = 0; j < 2; j++) {
	  flag = 0;
	  while (!flag) {
	    MPI_Testany (2, aReq, &index, &flag, aStatus);
	  }
	}
	break;

      case 7:
	/* use MPI_Testsome */
	j = 0;
	while (j < 2) {
	  outcount = 0;
	  while (!outcount) {
	    MPI_Testsome (2, aReq, &outcount, indices, aStatus);
	  }
	  j += outcount;
	}
	break;

      default:
	assert (0);
	break;
      }
    }
    else if (rank == 1) {
      /* set up receives for all of the sends */
      for (j = 0; j < 2; j++) {
	MPI_Irecv (&buf[j * BUF_SIZE], BUF_SIZE,
		   MPI_INT, 0, j, comm, &aReq[j]);
      }
      /* complete all of the receives... */
      MPI_Waitall (2, aReq, aStatus);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    /* free the persistent requests */
    for (i = 0 ; i < 2; i++) {
      MPI_Request_free (&aReq[i]);
    }
  }

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
