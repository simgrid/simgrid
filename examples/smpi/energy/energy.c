/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

int main(int argc, char *argv[])
{
  int rank;
  int i;
  char buf[1024];

  int err = MPI_Init(&argc, &argv);
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "MPI_init failed: %d\n", err);
    exit(EXIT_FAILURE);
  }

  err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "MPI_Comm_rank failed: %d", err);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    exit(EXIT_FAILURE);
  }

  int pstates = smpi_get_host_nb_pstates();

  char *s = buf;
  size_t sz = sizeof buf;
  size_t x = snprintf(s, sz,
               "[%.6f] [rank %d] Pstates: %d; Powers: %.0f",
               MPI_Wtime(), rank, pstates, smpi_get_host_power_peak_at(0));
  if (x < sz) {
    s += x;
    sz -= x;
  } else
    sz = 0;
  for (i = 1; i < pstates; i++) {
    x = snprintf(s, sz, ", %.0f", smpi_get_host_power_peak_at(i));
    if (x < sz) {
      s += x;
      sz -= x;
    } else
      sz = 0;
  }
  fprintf(stderr, "%s%s\n", buf, (sz ? "" : " [...]"));

  for (i = 0; i < pstates; i++) {
    smpi_set_host_pstate(i);
    fprintf(stderr, "[%.6f] [rank %d] Current pstate: %d; Current power: %.0f\n",
            MPI_Wtime(), rank, i, smpi_get_host_current_power_peak());

    SMPI_SAMPLE_FLOPS(1e9) {
      /* imagine here some code running for 1e9 flops... */
    }

    fprintf(stderr, "[%.6f] [rank %d] Energy consumed: %g Joules.\n",
            MPI_Wtime(), rank, smpi_get_host_consumed_energy());
  }

  err = MPI_Finalize();
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "MPI_Finalize failed: %d\n", err);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
