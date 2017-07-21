/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov) */

/* op-no-error.c -- construct some MPI_Ops and free them */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/op-no-error.c,v 1.1 2002/05/29 16:09:50 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"

/* construct multiple instances of same op to exercise more Umpire code... */
#define OP_COUNT  5


typedef struct {
  double real, imag;
} Complex;

void
myProd (void *inp, void *inoutp, int *len, MPI_Datatype *dptr)
{
  int i;
  Complex c;
  Complex *in = (Complex *) inp;
  Complex *inout = (Complex *) inoutp;

  for (i =0; i < *len; ++i) {
    c.real = inout->real*in->real - inout->imag*in->imag;
    c.imag = inout->real*in->imag + inout->imag*in->real;
    *inout = c;
    in++; inout++;
  }

  return;
}


int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  int i;
  char processor_name[128];
  int namelen = 128;
  MPI_Op newop[OP_COUNT];
  MPI_Op newop2[OP_COUNT];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  MPI_Barrier (comm);

  for (i = 0; i < OP_COUNT; i++)
    MPI_Op_create (myProd, 1, &newop[i]);

  for (i = 0; i < OP_COUNT; i++)
    MPI_Op_free (&newop[i]);

  MPI_Barrier (comm);

  /* now with an alias... */
  for (i = 0; i < OP_COUNT; i++)
    MPI_Op_create (myProd, 1, &newop[i]);

  for (i = 0; i < OP_COUNT; i++) {
    newop2[i] = newop[i];
    MPI_Op_free (&newop2[i]);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
