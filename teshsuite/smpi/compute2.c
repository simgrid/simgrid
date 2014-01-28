/* Copyright (c) 2009-2010, 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int i, n;
  double d;
  MPI_Init(&argc, &argv);
  n = argc > 1 ? atoi(argv[1]) : 0;
  d = 2.0;
  /* Run it only once across the whole set of processes */
  SMPI_SAMPLE_GLOBAL(1, -1) {
    for (i = 0; i < n; i++) {
      if (d < 10000) {
        d = d * d;
      } else {
        d = 2;
      }
    }
    printf("%d %f\n", i, d);
  }
  MPI_Finalize();
  return 0;
}
