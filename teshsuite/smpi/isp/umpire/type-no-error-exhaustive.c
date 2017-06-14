/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive.c -- use all type constructors correctly */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-error-exhaustive.c,v 1.1 2002/05/29 16:09:50 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpi.h"


#define TYPE_CONSTRUCTOR_COUNT 6

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int i;
  int blocklens[2], displs[2];
  MPI_Datatype newtype[TYPE_CONSTRUCTOR_COUNT];
  MPI_Datatype newtype2[TYPE_CONSTRUCTOR_COUNT];

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  newtype2[0] = MPI_DOUBLE;
  newtype2[1] = MPI_CHAR;
  blocklens[0] = blocklens[1] = 1;
  displs[0] = 0;
  displs[1] = 8;

  MPI_Barrier (comm);

  /* create the types */
  MPI_Type_struct (2, blocklens, displs, newtype2, &newtype[0]);
  MPI_Type_vector (2, 3, 4, newtype[0], &newtype[1]);
  MPI_Type_hvector (3, 2, 192, newtype[1], &newtype[2]);
  displs[1] = 2;
  MPI_Type_indexed (2, blocklens, displs, newtype[2], &newtype[3]);
  displs[1] = 512;
  MPI_Type_hindexed (2, blocklens, displs, newtype[3], &newtype[4]);
  displs[1] = 8;
  MPI_Type_contiguous (10, newtype[4], &newtype[5]);

  for (i = 0; i < TYPE_CONSTRUCTOR_COUNT; i++)
    MPI_Type_free (&newtype[i]);

  MPI_Barrier (comm);

  /* create the types again and free with an alias... */
  MPI_Type_struct (2, blocklens, displs, newtype2, &newtype[0]);
  MPI_Type_vector (2, 3, 4, newtype[0], &newtype[1]);
  MPI_Type_hvector (3, 2, 192, newtype[1], &newtype[2]);
  displs[1] = 2;
  MPI_Type_indexed (2, blocklens, displs, newtype[2], &newtype[3]);
  displs[1] = 512;
  MPI_Type_hindexed (2, blocklens, displs, newtype[3], &newtype[4]);
  displs[1] = 8;
  MPI_Type_contiguous (10, newtype[4], &newtype[5]);

  for (i = 0; i < TYPE_CONSTRUCTOR_COUNT; i++) {
    newtype2[i] = newtype[i];
    MPI_Type_free (&newtype2[i]);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
