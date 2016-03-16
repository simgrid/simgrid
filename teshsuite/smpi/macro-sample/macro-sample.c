/* Copyright (c) 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example should be instructive to learn about SMPI_SAMPLE_LOCAL and
   SMPI_SAMPLE_GLOBAL macros for execution sampling */

#include <stdio.h>
#include <mpi.h>

static double compute(double d0)
{
  double d = d0;
  int j;
  for (j = 0; j < 100 * 1000 * 1000; j++) { /* 100 kflop */
    if (d < 100000) {
      d = d * d;
    } else {
      d = 2;
    }
  }
  return d;
}

int main(int argc, char *argv[])
{
  int verbose;
  int i, n;
  double d;
  MPI_Init(&argc, &argv);
  verbose = argc <= 1;
  MPI_Comm_size(MPI_COMM_WORLD, &n);
  d = 2.0;
  for (i = 0; i < 5; i++) {
    /* I want no more than n + 1 benchs (thres < 0) */
    SMPI_SAMPLE_GLOBAL(n + 1, -1) {
      if (verbose)
        fprintf(stderr, "(%12.6f) [rank:%d]", MPI_Wtime(), smpi_process_index());
      else
        fprintf(stderr, "(0)");
      fprintf(stderr, " Run the first computation. It's globally benched, "
              "and I want no more than %d benchmarks (thres<0)\n", n + 1);
      d = compute(2.0);
     }
  }

  n = 0;
  for (i = 0; i < 5; i++) {
    /* I want the standard error to go below 0.1 second.
     * Two tests at least will be run (count is not > 0) */
    SMPI_SAMPLE_LOCAL(0, 0.1) {
      if (verbose || n++ < 2) {
        if (verbose)
          fprintf(stderr, "(%12.6f)", MPI_Wtime());
        else
          fprintf(stderr, "(1)");
        fprintf(stderr,
                " [rank:%d] Run the first (locally benched) computation. It's locally benched, and I want the "
                "standard error to go below 0.1 second (count is not >0)\n", smpi_process_index());
      }
      d = compute(d);
     }
  }

  if (verbose)
    fprintf(stderr, "(%12.6f) [rank:%d] The result of the computation is: %f\n", MPI_Wtime(), smpi_process_index(), d);
  else
    fprintf(stderr, "(2) [rank:%d] Done.\n", smpi_process_index());

  MPI_Finalize();
  return 0;
}
