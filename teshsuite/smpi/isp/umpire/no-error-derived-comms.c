/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Thu Nov 30 2000 */
/* no-error-derived-comms.c -- do some MPI calls without any errors */

#include <stdio.h>
#include "mpi.h"


#define RUN_COMM_DUP
#define RUN_COMM_CREATE
#define RUN_INTERCOMM_CREATE
#define RUN_CART_CREATE
#define RUN_GRAPH_CREATE
#define RUN_CART_SUB
#define RUN_INTERCOMM_MERGE


#define buf_size 128
#define DCOMM_CALL_COUNT  7 /* MPI_Cart_create; MPI_Cart_sub;
			       MPI_Comm_create; MPI_Comm_dup;
			       MPI_Comm_split; MPI_Graph_create;
			       and MPI_Intercomm_merge; store
			       MPI_Intercomm_create separately... */
#define TWOD     2
#define GRAPH_SZ 4
#define INTERCOMM_CREATE_TAG 666


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  int i, j;
  int *granks;
  char processor_name[128];
  int namelen = 128;
  int buf[buf_size];
  MPI_Status status;
  MPI_Comm temp;
  MPI_Comm intercomm = MPI_COMM_NULL;
  MPI_Comm dcomms[DCOMM_CALL_COUNT];
  MPI_Group world_group, dgroup;
  int intersize, dnprocs[DCOMM_CALL_COUNT], drank[DCOMM_CALL_COUNT];
  int dims[TWOD], periods[TWOD], remain_dims[TWOD];
  int graph_index[] = { 2, 3, 4, 6 };
  int graph_edges[] = { 1, 3, 0, 3, 0, 2 };

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (MPI_COMM_WORLD);

  /* probably want number to be higher... */
  if (nprocs < 4) {
      printf ("not enough tasks\n");
  }
  else {
    if (DCOMM_CALL_COUNT > 0) {
#ifdef RUN_COMM_DUP
      /* create all of the derived communicators... */
      /* simplest is created by MPI_Comm_dup... */
      MPI_Comm_dup (MPI_COMM_WORLD, &dcomms[0]);
#else
      dcomms[0] = MPI_COMM_NULL;
#endif
    }

    if (DCOMM_CALL_COUNT > 1) {
#ifdef RUN_COMM_CREATE
      /* use subset of MPI_COMM_WORLD group for MPI_Comm_create... */
      MPI_Comm_group (MPI_COMM_WORLD, &world_group);
      granks = (int *) malloc (sizeof(int) * (nprocs/2));
      for (i = 0; i < nprocs/2; i++)
	granks [i] = 2 * i;
      MPI_Group_incl (world_group, nprocs/2, granks, &dgroup);
      MPI_Comm_create (MPI_COMM_WORLD, dgroup, &dcomms[1]);
      MPI_Group_free (&world_group);
      MPI_Group_free (&dgroup);
      free (granks);
#else
      dcomms[1] = MPI_COMM_NULL;
#endif
    }

    if (DCOMM_CALL_COUNT > 2) {
#ifdef RUN_COMM_SPLIT
      /* split into thirds with inverted ranks... */
      MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &dcomms[2]);
#else
      dcomms[2] = MPI_COMM_NULL;
#endif
    }

#ifdef RUN_INTERCOMM_CREATE
    if ((DCOMM_CALL_COUNT < 2) || (dcomms[2] == MPI_COMM_NULL)) {
      MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &temp);
    }
    else {
      temp = dcomms[2];
    }
    if (rank % 3) {
      MPI_Intercomm_create (temp, 0, MPI_COMM_WORLD,
			    (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			    nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			    INTERCOMM_CREATE_TAG, &intercomm);
    }
    if ((DCOMM_CALL_COUNT < 2) || (dcomms[2] == MPI_COMM_NULL)) {
      MPI_Comm_free (&temp);
    }
#endif

    if (DCOMM_CALL_COUNT > 3) {
#ifdef RUN_CART_CREATE
      /* create a 2 X nprocs/2 torus topology, allow reordering */
      dims[0] = 2;
      dims[1] = nprocs/2;
      periods[0] = periods[1] = 1;
      MPI_Cart_create (MPI_COMM_WORLD, TWOD, dims, periods, 1, &dcomms[3]);
#else
      dcomms[3] = MPI_COMM_NULL;
#endif
    }

    if (DCOMM_CALL_COUNT > 4) {
#ifdef RUN_GRAPH_CREATE
      /* create the graph on p.268 MPI: The Complete Reference... */
      MPI_Graph_create (MPI_COMM_WORLD, GRAPH_SZ,
			graph_index, graph_edges, 1, &dcomms[4]);
#else
      dcomms[4] = MPI_COMM_NULL;
#endif
    }

    if (DCOMM_CALL_COUNT > 5) {
#ifdef RUN_CART_SUB
#ifndef RUN_CART_CREATE
      /* need to make cartesian communicator temporarily... */
      /* create a 2 X nprocs/2 torus topology, allow reordering */
      dims[0] = 2;
      dims[1] = nprocs/2;
      periods[0] = periods[1] = 1;
      MPI_Cart_create (MPI_COMM_WORLD, TWOD, dims, periods, 1, &dcomms[3]);
#endif
      if (dcomms[3] != MPI_COMM_NULL) {
	/* create 2 1 X nprocs/2 topologies... */
	remain_dims[0] = 0;
	remain_dims[1] = 1;
	MPI_Cart_sub (dcomms[3], remain_dims, &dcomms[5]);
#ifndef RUN_CART_CREATE
	/* free up temporarily created cartesian communicator... */
	MPI_Comm_free (&dcomms[3]);
#endif
      }
      else {
	dcomms[5] = MPI_COMM_NULL;
      }
#else
      dcomms[5] = MPI_COMM_NULL;
#endif
    }

    if (DCOMM_CALL_COUNT > 6) {
#ifdef RUN_INTERCOMM_MERGE
#ifndef RUN_INTERCOMM_CREATE
#ifndef RUN_COMM_SPLIT
      /* need to make split communicator temporarily... */
      /* split into thirds with inverted ranks... */
      MPI_Comm_split (MPI_COMM_WORLD, rank % 3, nprocs - rank, &dcomms[2]);
#endif
#endif
      /* create an intercommunicator and merge it... */
      if (rank % 3) {
#ifndef RUN_INTERCOMM_CREATE
	MPI_Intercomm_create (dcomms[2], 0, MPI_COMM_WORLD,
			      (((nprocs % 3) == 2) && ((rank % 3) == 2)) ?
			      nprocs - 1 : nprocs - (rank % 3) - (nprocs % 3),
			      INTERCOMM_CREATE_TAG, &intercomm);
#endif

	MPI_Intercomm_merge (intercomm, ((rank % 3) == 1), &dcomms[6]);

#ifndef RUN_INTERCOMM_CREATE
	/* we are done with intercomm... */
	MPI_Comm_free (&intercomm);
#endif
      }
      else {
	dcomms[6] = MPI_COMM_NULL;
      }
#ifndef RUN_INTERCOMM_CREATE
#ifndef RUN_COMM_SPLIT
      if (dcomms[2] != MPI_COMM_NULL)
	/* free up temporarily created split communicator... */
	MPI_Comm_free (&dcomms[2]);
#endif
#endif
#else
      dcomms[6] = MPI_COMM_NULL;
#endif
    }

    /* get all of the sizes and ranks... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      if (dcomms[i] != MPI_COMM_NULL) {
	MPI_Comm_size (dcomms[i], &dnprocs[i]);
	MPI_Comm_rank (dcomms[i], &drank[i]);
      }
      else {
	dnprocs[i] = 0;
	drank[i] = -1;
      }
    }

#ifdef RUN_INTERCOMM_CREATE
    /* get the intercomm remote size... */
    if (rank % 3) {
      MPI_Comm_remote_size (intercomm, &intersize);
    }
#endif

    /* do some point to point on all of the dcomms... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      if (dnprocs[i] > 1) {
	if (drank[i] == 0) {
	  for (j = 1; j < dnprocs[i]; j++) {
	    MPI_Recv (buf, buf_size, MPI_INT, j, 0, dcomms[i], &status);
	  }
	}
	else {
	  memset (buf, 1, buf_size*sizeof(int));

	  MPI_Send (buf, buf_size, MPI_INT, 0, 0, dcomms[i]);
	}
      }
    }

#ifdef RUN_INTERCOMM_CREATE
    /* do some point to point on the intercomm... */
    if ((rank % 3) == 1) {
      for (j = 0; j < intersize; j++) {
	MPI_Recv (buf, buf_size, MPI_INT, j, 0, intercomm, &status);
      }
    }
    else if ((rank % 3) == 2) {
      for (j = 0; j < intersize; j++) {
	memset (buf, 1, buf_size*sizeof(int));

	MPI_Send (buf, buf_size, MPI_INT, j, 0, intercomm);
      }
    }
#endif

    /* do a bcast on all of the dcomms... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      /* IBM's implementation gets error with comm over MPI_COMM_NULL... */
      if (dnprocs[i] > 0)
	MPI_Bcast (buf, buf_size, MPI_INT, 0, dcomms[i]);
    }

    /* use any source receives... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      if (dnprocs[i] > 1) {
	if (drank[i] == 0) {
	  for (j = 1; j < dnprocs[i]; j++) {
	    MPI_Recv (buf, buf_size, MPI_INT,
		      MPI_ANY_SOURCE, 0, dcomms[i], &status);
	  }
	}
	else {
	  memset (buf, 1, buf_size*sizeof(int));

	  MPI_Send (buf, buf_size, MPI_INT, 0, 0, dcomms[i]);
	}
      }
    }

#ifdef RUN_INTERCOMM_CREATE
    /* do any source receives on the intercomm... */
    if ((rank % 3) == 1) {
      for (j = 0; j < intersize; j++) {
	MPI_Recv (buf, buf_size, MPI_INT,
		  MPI_ANY_SOURCE, 0, intercomm, &status);
      }
    }
    else if ((rank % 3) == 2) {
      for (j = 0; j < intersize; j++) {
	memset (buf, 1, buf_size*sizeof(int));

	MPI_Send (buf, buf_size, MPI_INT, j, 0, intercomm);
      }
    }
#endif

    /* do a barrier on all of the dcomms... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      /* IBM's implementation gets with communication over MPI_COMM_NULL... */
      if (dnprocs[i] > 0)
	MPI_Barrier (dcomms[i]);
    }

    /* free all of the derived communicators... */
    for (i = 0; i < DCOMM_CALL_COUNT; i++) {
      /* freeing MPI_COMM_NULL is explicitly defined as erroneous... */
      if (dnprocs[i] > 0)
	MPI_Comm_free (&dcomms[i]);
    }

#ifdef RUN_INTERCOMM_CREATE
    if (rank % 3)
      /* we are done with intercomm... */
      MPI_Comm_free (&intercomm);
#endif
  }

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* EOF */
