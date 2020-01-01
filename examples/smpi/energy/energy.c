/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

#include <simgrid/host.h>
#include <simgrid/plugins/energy.h>

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

  int pstates = sg_host_get_nb_pstates(sg_host_self());

  char *s = buf;
  size_t sz = sizeof buf;
  size_t x = snprintf(s, sz,
               "[%.6f] [rank %d] Pstates: %d; Powers: %.0f",
               MPI_Wtime(), rank, pstates, sg_host_get_pstate_speed(sg_host_self(), 0));
  if (x < sz) {
    s += x;
    sz -= x;
  } else
    sz = 0;
  for (i = 1; i < pstates; i++) {
    x = snprintf(s, sz, ", %.0f", sg_host_get_pstate_speed(sg_host_self(), i));
    if (x < sz) {
      s += x;
      sz -= x;
    } else
      sz = 0;
  }
  fprintf(stderr, "%s%s\n", buf, (sz ? "" : " [...]"));

  for (i = 0; i < pstates; i++) {
    sg_host_set_pstate(sg_host_self(), i);
    fprintf(stderr, "[%.6f] [rank %d] Current pstate: %d; Current power: %.0f\n",
            MPI_Wtime(), rank, i, sg_host_speed(sg_host_self()));

    SMPI_SAMPLE_FLOPS(1e9) {
      /* imagine here some code running for 1e9 flops... */
    }

    fprintf(stderr, "[%.6f] [rank %d] Energy consumed: %g Joules.\n",
            MPI_Wtime(), rank,  sg_host_get_consumed_energy(sg_host_self()));
  }

  err = MPI_Finalize();
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "MPI_Finalize failed: %d\n", err);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
