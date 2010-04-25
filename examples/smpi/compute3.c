/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int i;
  double d;
  MPI_Init(&argc, &argv);
  d = 2.0;
/*  SMPI_DO_ONCE */{
    for (i = 0; i < atoi(argv[1]); i++) {
      if (d < 10000) {
        d = d * d;
      } else {
        d = 2;
      }
    }
    printf("%d %f\n", i, d);
  }
/*  SMPI_DO_ONCE */{
    for (i = 0; i < 2 * atoi(argv[1]); i++) {
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
