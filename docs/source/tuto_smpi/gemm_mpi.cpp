/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "stdio.h"
#include "mpi.h"

const int size = 3000;

float a[size][size];
float b[size][size];
float c[size][size];

void multiply(int istart, int iend)
{
    for (int i = istart; i <= iend; ++i){
        for (int j = 0; j < size; ++j) {
            for (int k = 0; k < size; ++k) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

int main(int argc, char* argv[])
{
    int rank, nproc;
    int istart, iend;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // MPI_Barrier(MPI_COMM_WORLD);
    // start = MPI_Wtime();

    if (rank == 0) {
        // Initialize buffers.
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                a[i][j] = (float)i + j;
                b[i][j] = (float)i - j;
                c[i][j] = 0.0f;
            }
        }
    }

    // Broadcast matrices to all workers.
    MPI_Bcast(a, size*size, MPI_FLOAT, 0,MPI_COMM_WORLD);
    MPI_Bcast(b, size*size, MPI_FLOAT, 0,MPI_COMM_WORLD);
    MPI_Bcast(c, size*size, MPI_FLOAT, 0,MPI_COMM_WORLD);

    // Partition work by i-for-loop.
    istart = (size / nproc) * rank;
    iend = (size / nproc) * (rank + 1) - 1;

    // Compute matrix multiplication in [istart,iend]
    // of i-for-loop.
    // C <- C + A x B
    multiply(istart, iend);

    // Gather computed results.
    MPI_Gather(c + (size/nproc*rank),
               size*size/nproc,
               MPI_FLOAT,
               c + (size/nproc*rank),
               size*size/nproc,
               MPI_FLOAT,
               0,
               MPI_COMM_WORLD);

    if (rank == 0) {
        // Compute remaining multiplications
        // when size % nproc > 0.
        if (size % nproc > 0) {
            multiply((size/nproc)*nproc, size-1);
        }
    }

    // MPI_Barrier(MPI_COMM_WORLD);
    // end = MPI_Wtime();

    MPI_Finalize();

    // if (rank == 0) { /* use time on master node */
    //     float msec_total = 0.0f;

    //     // Compute and print the performance
    //     float msec_per_matrix_mul = end-start;
    //     double flops_per_matrix_mul = 2.0 * (double)size * (double)size * (double)size;
    //     double giga_flops = (flops_per_matrix_mul * 1.0e-9f) / (msec_per_matrix_mul / 1000.0f);
    //     printf(
    //         "Performance= %.2f GFlop/s, Time= %.3f msec, Size= %.0f Ops\n",
    //         giga_flops,
    //         msec_per_matrix_mul,
    //         flops_per_matrix_mul);
    // }

    return 0;
}
