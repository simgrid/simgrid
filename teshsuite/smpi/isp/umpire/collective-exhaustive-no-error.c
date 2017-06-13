/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) Thu Oct 17 2002 */
/* collective-exhaustive-no-error.c -- do some many collective operations */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/collective-exhaustive-no-error.c,v 1.1 2002/10/24 17:04:54 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"


#define RUN_BARRIER
#define RUN_BCAST
#define RUN_GATHER
#define RUN_GATHERV
#define RUN_SCATTER
#define RUN_SCATTERV
#define RUN_ALLGATHER
#define RUN_ALLGATHERV
#define RUN_ALLTOALL
#define RUN_ALLTOALLV
#define RUN_REDUCE
#define RUN_ALLREDUCE
#define RUN_REDUCE_SCATTER
#define RUN_SCAN


#define RUN_MAX
#define RUN_MIN
#define RUN_SUM
#define RUN_PROD
#define RUN_LAND
#define RUN_BAND
#define RUN_LOR
#define RUN_BOR
#define RUN_LXOR
#define RUN_BXOR
#define RUN_MAXLOC
#define RUN_MINLOC
#define RUN_USEROP


#define buf_size 128
#define OP_COUNT 10


#ifdef RUN_USEROP
typedef struct {
  double real, imag;
} complex_t;

void
complex_prod (void *inp, void *inoutp, int *len, MPI_Datatype *dptr)
{
  int i;
  complex_t c;
  complex_t *in = (complex_t *) inp;
  complex_t *inout = (complex_t *) inoutp;

  for (i = 0; i < *len; i++) {
    c.real = inout->real * in->real - inout->imag * in->imag;
    c.imag = inout->real * in->imag + inout->imag * in->real;
    *inout = c;
    in++;
    inout++;
  }

  return;
}
#endif


int
main (int argc, char **argv)
{
  int i, nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int *buf0, *buf1, *displs, *counts, *rcounts, *alltoallvcounts;
#ifdef RUN_USEROP
  MPI_Op user_op;
  complex_t *a, *answer;
  MPI_Datatype ctype;
#endif

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  buf0 = (int *) malloc (buf_size * nprocs * sizeof(int));
  assert (buf0);
  for (i = 0; i < buf_size * nprocs; i++)
    buf0[i] = rank;

#ifdef RUN_ALLTOALLV
  alltoallvcounts = (int *) malloc (nprocs * sizeof(int));
  assert (alltoallvcounts);
  for (i = 0; i < nprocs; i++)
    if ((i + rank) < buf_size)
      alltoallvcounts[i] = i + rank;
    else
      alltoallvcounts[i] = buf_size;
#endif

#ifdef RUN_USEROP
  a = (complex_t *) malloc (buf_size * nprocs * sizeof(complex_t));
  for (i = 0; i < buf_size * nprocs; i++) {
    a[i].real = ((double) rank)/((double) nprocs);
    a[i].imag = ((double) (-rank))/((double) nprocs);
  }

  MPI_Type_contiguous (2, MPI_DOUBLE, &ctype);
  MPI_Type_commit (&ctype);

  MPI_Op_create (complex_prod, 1 /* TRUE */, &user_op);
#endif

  if (rank == 0) {
    buf1 = (int *) malloc (buf_size * nprocs * sizeof(int));
    assert (buf1);
    for (i = 0; i < buf_size * nprocs; i++)
      buf1[i] = i;

    displs = (int *) malloc (nprocs * sizeof(int));
    counts = (int *) malloc (nprocs * sizeof(int));
    rcounts = (int *) malloc (nprocs * sizeof(int));
    assert (displs && counts && rcounts);
    for (i = 0; i < nprocs; i++) {
      displs[i] = i * buf_size;
      if (i < buf_size)
	rcounts[i] = counts[i] = i;
      else
	rcounts[i] = counts[i] = buf_size;
    }

#ifdef RUN_USEROP
    answer = (complex_t *) malloc (buf_size * nprocs * sizeof(complex_t));
#endif
  }

#ifdef RUN_BARRIER
  for (i = 0; i < OP_COUNT; i++)
    MPI_Barrier (comm);
#endif

#ifdef RUN_BCAST
  for (i = 0; i < OP_COUNT; i++)
    MPI_Bcast (buf0, buf_size, MPI_INT, 0, comm);
#endif

#ifdef RUN_GATHER
  for (i = 0; i < OP_COUNT; i++)
    MPI_Gather (&buf0[rank*buf_size], buf_size,
		MPI_INT, buf1, buf_size, MPI_INT, 0, comm);
#endif

#ifdef RUN_SCATTER
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scatter (buf1, buf_size, MPI_INT, buf0, buf_size, MPI_INT, 0, comm);
#endif

#ifdef RUN_GATHERV
  for (i = 0; i < OP_COUNT; i++)
    MPI_Gatherv (&buf0[rank*buf_size],
		 (rank < buf_size) ? rank : buf_size,
		 MPI_INT, buf1, rcounts, displs, MPI_INT, 0, comm);
#endif

#ifdef RUN_SCATTERV
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scatterv (buf1, counts, displs, MPI_INT, buf0,
		  (rank < buf_size) ? rank : buf_size, MPI_INT, 0, comm);
#endif

#ifdef RUN_REDUCE
#ifdef RUN_MAX
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_MAX, 0, comm);
#endif

#ifdef RUN_MIN
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_MIN, 0, comm);
#endif

#ifdef RUN_SUM
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_SUM, 0, comm);
#endif

#ifdef RUN_PROD
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_PROD, 0, comm);
#endif

#ifdef RUN_LAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_LAND, 0, comm);
#endif

#ifdef RUN_BAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_BAND, 0, comm);
#endif

#ifdef RUN_LOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_LOR, 0, comm);
#endif

#ifdef RUN_BOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_BOR, 0, comm);
#endif

#ifdef RUN_LXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_LXOR, 0, comm);
#endif

#ifdef RUN_BXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (buf0, buf1, buf_size, MPI_INT, MPI_BXOR, 0, comm);
#endif

#ifdef RUN_MAXLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Reduce (buf0, buf1, buf_size, MPI_2INT, MPI_MAXLOC, 0, comm);
  else
    fprintf (stderr, "Not enough tasks for MAXLOC test\n");
#endif

#ifdef RUN_MINLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Reduce (buf0, buf1, buf_size, MPI_2INT, MPI_MINLOC, 0, comm);
  else
    fprintf (stderr, "Not enough tasks for MINLOC test\n");
#endif

#ifdef RUN_USEROP
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce (a, answer, buf_size, ctype, user_op, 0, comm);
#endif
#endif

  if (rank != 0) {
    buf1 = (int *) malloc (buf_size * nprocs * sizeof(int));
    assert (buf1);
    for (i = 0; i < buf_size * nprocs; i++)
      buf1[i] = i;

    displs = (int *) malloc (nprocs * sizeof(int));
    counts = (int *) malloc (nprocs * sizeof(int));
    rcounts = (int *) malloc (nprocs * sizeof(int));
    assert (displs && counts && rcounts);
    for (i = 0; i < nprocs; i++) {
      displs[i] = i * buf_size;
      if (i < buf_size)
	rcounts[i] = counts[i] = i;
      else
	rcounts[i] = counts[i] = buf_size;
    }

#ifdef RUN_USEROP
    answer = (complex_t *) malloc (buf_size * nprocs * sizeof(complex_t));
#endif
  }

#ifdef RUN_ALLGATHER
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allgather (buf0, buf_size, MPI_INT, buf1, buf_size, MPI_INT, comm);
#endif

#ifdef RUN_ALLTOALL
  for (i = 0; i < OP_COUNT; i++)
    MPI_Alltoall (buf1, buf_size, MPI_INT, buf0, buf_size, MPI_INT, comm);
#endif

#ifdef RUN_ALLGATHERV
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allgatherv (buf0,
		    (rank < buf_size) ? rank : buf_size,
		    MPI_INT, buf1, rcounts, displs, MPI_INT, comm);
#endif

#ifdef RUN_ALLTOALLV
  for (i = 0; i < OP_COUNT; i++)
    MPI_Alltoallv (buf1, alltoallvcounts, displs, MPI_INT,
		   buf0, alltoallvcounts, displs, MPI_INT, comm);
#endif

#ifdef RUN_ALLREDUCE
#ifdef RUN_MAX
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_MAX, comm);
#endif

#ifdef RUN_MIN
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_MIN, comm);
#endif

#ifdef RUN_SUM
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_SUM, comm);
#endif

#ifdef RUN_PROD
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_PROD, comm);
#endif

#ifdef RUN_LAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_LAND, comm);
#endif

#ifdef RUN_BAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_BAND, comm);
#endif

#ifdef RUN_LOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_LOR, comm);
#endif

#ifdef RUN_BOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_BOR, comm);
#endif

#ifdef RUN_LXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_LXOR, comm);
#endif

#ifdef RUN_BXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (buf0, buf1, buf_size, MPI_INT, MPI_BXOR, comm);
#endif

#ifdef RUN_MAXLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Allreduce (buf0, buf1, buf_size, MPI_2INT, MPI_MAXLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MAXLOC test\n");
#endif

#ifdef RUN_MINLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Allreduce (buf0, buf1, buf_size, MPI_2INT, MPI_MINLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MINLOC test\n");
#endif

#ifdef RUN_USEROP
  for (i = 0; i < OP_COUNT; i++)
    MPI_Allreduce (a, answer, buf_size, ctype, user_op, comm);
#endif
#endif

#ifdef RUN_REDUCE_SCATTER
#ifdef RUN_MAX
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_MAX, comm);
#endif

#ifdef RUN_MIN
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_MIN, comm);
#endif

#ifdef RUN_SUM
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_SUM, comm);
#endif

#ifdef RUN_PROD
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_PROD, comm);
#endif

#ifdef RUN_LAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_LAND, comm);
#endif

#ifdef RUN_BAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_BAND, comm);
#endif

#ifdef RUN_LOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_LOR, comm);
#endif

#ifdef RUN_BOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_BOR, comm);
#endif

#ifdef RUN_LXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_LXOR, comm);
#endif

#ifdef RUN_BXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_INT, MPI_BXOR, comm);
#endif

#ifdef RUN_MAXLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_2INT, MPI_MAXLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MAXLOC test\n");
#endif

#ifdef RUN_MINLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Reduce_scatter (buf0, buf1, rcounts, MPI_2INT, MPI_MINLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MINLOC test\n");
#endif

#ifdef RUN_USEROP
  for (i = 0; i < OP_COUNT; i++)
    MPI_Reduce_scatter (a, answer, rcounts, ctype, user_op, comm);
#endif
#endif

#ifdef RUN_SCAN
#ifdef RUN_MAX
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_MAX, comm);
#endif

#ifdef RUN_MIN
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_MIN, comm);
#endif

#ifdef RUN_SUM
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_SUM, comm);
#endif

#ifdef RUN_PROD
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_PROD, comm);
#endif

#ifdef RUN_LAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_LAND, comm);
#endif

#ifdef RUN_BAND
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_BAND, comm);
#endif

#ifdef RUN_LOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_LOR, comm);
#endif

#ifdef RUN_BOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_BOR, comm);
#endif

#ifdef RUN_LXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_LXOR, comm);
#endif

#ifdef RUN_BXOR
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (buf0, buf1, buf_size, MPI_INT, MPI_BXOR, comm);
#endif

#ifdef RUN_MAXLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Scan (buf0, buf1, buf_size, MPI_2INT, MPI_MAXLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MAXLOC test\n");
#endif

#ifdef RUN_MINLOC
  if (nprocs > 1)
    for (i = 0; i < OP_COUNT; i++)
      MPI_Scan (buf0, buf1, buf_size, MPI_2INT, MPI_MINLOC, comm);
  else
    fprintf (stderr, "Not enough tasks for MINLOC test\n");
#endif

#ifdef RUN_USEROP
  for (i = 0; i < OP_COUNT; i++)
    MPI_Scan (a, answer, buf_size, ctype, user_op, comm);
#endif
#endif

#ifdef RUN_BARRIER
  MPI_Barrier (comm);
#endif

#ifdef RUN_USEROP
  free (a);
  free (answer);
  MPI_Op_free (&user_op);
  MPI_Type_free (&ctype);
#endif

#ifdef RUN_ALLTOALLV
  free (alltoallvcounts);
#endif

  free (buf0);
  free (buf1);
  free (displs);
  free (counts);
  free (rcounts);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);

  return 0;
}

/* EOF */
