/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */
/* change-send-buffer-exhaustive.c -- do lots pt-2-pt ops with */
/*                                    send buffer overwrite errors*/

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/change-send-buffer-exhaustive.c,v 1.5 2002/10/24 17:04:54 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"

#define BUF_SIZE 128
#define NUM_SEND_TYPES 8
#define NUM_PERSISTENT_SEND_TYPES 4
#define NUM_BSEND_TYPES 2
#define NUM_COMPLETION_MECHANISMS 8

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int bbuf[(BUF_SIZE + MPI_BSEND_OVERHEAD) * 2 * NUM_BSEND_TYPES];
  int buf[BUF_SIZE * 2 * NUM_SEND_TYPES];
  int i, j, k, at_size, send_t_number, index, outcount, total, flag;
  int num_errors, error_count, indices[2 * NUM_SEND_TYPES];
  MPI_Request aReq[2 * NUM_SEND_TYPES];
  MPI_Status aStatus[2 * NUM_SEND_TYPES];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Buffer_attach (bbuf, sizeof(int) *
		     (BUF_SIZE + MPI_BSEND_OVERHEAD) * 2 * NUM_BSEND_TYPES);

  if (rank == 0) {
    /* set up persistent sends... */
    send_t_number = NUM_SEND_TYPES - NUM_PERSISTENT_SEND_TYPES;

    MPI_Send_init (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		    1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
    MPI_Send_init (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		    BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		    comm, &aReq[send_t_number * 2 + 1]);

    send_t_number++;

    MPI_Bsend_init (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		    1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
    MPI_Bsend_init (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		    BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		    comm, &aReq[send_t_number * 2 + 1]);


    send_t_number++;

    MPI_Rsend_init (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		    1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
    MPI_Rsend_init (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		    BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		    comm, &aReq[send_t_number * 2 + 1]);

    send_t_number++;

    MPI_Ssend_init (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		    1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
    MPI_Ssend_init (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		    BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		    comm, &aReq[send_t_number * 2 + 1]);
  }

  for (k = 0; k < (NUM_COMPLETION_MECHANISMS * 2); k++) {
    if (rank == 0) {
      /* initialize all of the send buffers */
      for (j = 0; j < NUM_SEND_TYPES; j++) {
	for (i = 0; i < BUF_SIZE; i++) {
	  buf[2 * j * BUF_SIZE + i] = i;
	  buf[((2 * j + 1) * BUF_SIZE) + i] = BUF_SIZE - 1 - i;
	}
      }
    }
    else if (rank == 1) {
      /* zero out all of the receive buffers */
      bzero (buf, sizeof(int) * BUF_SIZE * 2 * NUM_SEND_TYPES);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
      /* set up transient sends... */
      send_t_number = 0;

      MPI_Isend (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		 1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
      MPI_Isend (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		 BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		 comm, &aReq[send_t_number * 2 + 1]);

      send_t_number++;

      MPI_Ibsend (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		  1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
      MPI_Ibsend (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		  BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		  comm, &aReq[send_t_number * 2 + 1]);

      send_t_number++;

      /* Barrier to ensure receives are posted for rsends... */
      MPI_Barrier(MPI_COMM_WORLD);

      MPI_Irsend (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		  1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
      MPI_Irsend (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		  BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		  comm, &aReq[send_t_number * 2 + 1]);

      send_t_number++;

      MPI_Issend (&buf[send_t_number * 2 * BUF_SIZE], BUF_SIZE, MPI_INT,
		  1, send_t_number * 2, comm, &aReq[send_t_number * 2]);
      MPI_Issend (&buf[(send_t_number * 2 + 1) * BUF_SIZE],
		  BUF_SIZE, MPI_INT, 1, send_t_number * 2 + 1,
		  comm, &aReq[send_t_number * 2 + 1]);

      /* just to be paranoid */
      send_t_number++;
      assert (send_t_number == NUM_SEND_TYPES - NUM_PERSISTENT_SEND_TYPES);

      /* start the persistent sends... */
      if (k % 2) {
	MPI_Startall (NUM_PERSISTENT_SEND_TYPES * 2, &aReq[2 * send_t_number]);
      }
      else {
	for (j = 0; j < NUM_PERSISTENT_SEND_TYPES * 2; j++) {
	  MPI_Start (&aReq[2 * send_t_number + j]);
	}
      }

      /* NOTE: Changing the send buffer of a Bsend is NOT an error... */
      for (j = 0; j < NUM_SEND_TYPES; j++) {
	/* muck the buffers */
	buf[j * 2 * BUF_SIZE + (BUF_SIZE >> 1)] = BUF_SIZE;
      }

      printf ("USER MSG: 6 change send buffer errors in iteration #%d:\n", k);

      /* complete the sends */
      switch (k/2) {
      case 0:
	/* use MPI_Wait */
	for (j = 0; j < NUM_SEND_TYPES * 2; j++) {
	  MPI_Wait (&aReq[j], &aStatus[j]);
	}
	break;

      case 1:
	/* use MPI_Waitall */
	MPI_Waitall (NUM_SEND_TYPES * 2, aReq, aStatus);
	break;

      case 2:
	/* use MPI_Waitany */
	for (j = 0; j < NUM_SEND_TYPES * 2; j++) {
	  MPI_Waitany (NUM_SEND_TYPES * 2, aReq, &index, aStatus);
	}

	break;

      case 3:
	/* use MPI_Waitsome */
	total = 0;
	while (total < NUM_SEND_TYPES * 2) {
	  MPI_Waitsome (NUM_SEND_TYPES * 2, aReq, &outcount, indices, aStatus);

	  total += outcount;
	}

	break;

      case 4:
	/* use MPI_Test */
	for (j = 0; j < NUM_SEND_TYPES * 2; j++) {
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
	  MPI_Testall (NUM_SEND_TYPES * 2, aReq, &flag, aStatus);
	}

	break;

      case 6:
	/* use MPI_Testany */
	for (j = 0; j < NUM_SEND_TYPES * 2; j++) {
	  flag = 0;
	  while (!flag) {
	    MPI_Testany (NUM_SEND_TYPES * 2, aReq, &index, &flag, aStatus);
	  }
	}

	break;

      case 7:
	/* use MPI_Testsome */
	total = 0;
	while (total < NUM_SEND_TYPES * 2) {
	  outcount = 0;

	  while (!outcount) {
	    MPI_Testsome (NUM_SEND_TYPES * 2, aReq,
			  &outcount, indices, aStatus);
	  }

	  total += outcount;
	}

	break;

      default:
	assert (0);
	break;
      }
    }
    else if (rank == 1) {
      /* set up receives for all of the sends */
      for (j = 0; j < NUM_SEND_TYPES * 2; j++) {
	MPI_Irecv (&buf[j * BUF_SIZE], BUF_SIZE,
		   MPI_INT, 0, j, comm, &aReq[j]);
      }

      /* Barrier to ensure receives are posted for rsends... */
      MPI_Barrier(MPI_COMM_WORLD);

      /* complete all of the receives... */
      MPI_Waitall (NUM_SEND_TYPES * 2, aReq, aStatus);
    }
    else {
      /* Barrier to ensure receives are posted for rsends... */
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    /* free the persistent requests */
    for (i = 2* (NUM_SEND_TYPES - NUM_PERSISTENT_SEND_TYPES);
	 i < 2 * NUM_SEND_TYPES; i++) {
      MPI_Request_free (&aReq[i]);
    }
  }

  MPI_Buffer_detach (bbuf, &at_size);

  assert (at_size ==
	  sizeof(int) * (BUF_SIZE + MPI_BSEND_OVERHEAD) * 2 * NUM_BSEND_TYPES);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
