/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*==================================================================================================*/
/*# This file is part of the CodeVault project. The project is licensed under Apache Version 2.0.*/
/*# CodeVault is part of the EU-project PRACE-4IP (WP7.3.C).*/
/*#*/
/*# Author(s):*/
/*#   Valeriu Codreanu <valeriu.codreanu@surfsara.nl>*/
/*#*/
/*# ==================================================================================================*/

#include "stdio.h"
#include "mpi.h"

void multiply(float* a, float* b, float* c, int istart, int iend, int size);
void multiply_sampled(float* a, float* b, float* c, int istart, int iend, int size);


void multiply(float* a, float* b, float* c, int istart, int iend, int size)
{
    for (int i = istart; i <= iend; ++i) {
        for (int j = 0; j < size; ++j) {
            for (int k = 0; k < size; ++k) {
                c[i*size+j] += a[i*size+k] * b[k*size+j];
            }
        }
    }
}

void multiply_sampled(float* a, float* b, float* c, int istart, int iend, int size)
{
    //for (int i = istart; i <= iend; ++i) {
    SMPI_SAMPLE_GLOBAL (int i = istart, i <= iend, ++i, 10, 0.005){
        for (int j = 0; j < size; ++j) {
            for (int k = 0; k < size; ++k) {
                c[i*size+j] += a[i*size+k] * b[k*size+j];
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

    if(argc<2){
      if (rank == 0)
        printf("Usage : gemm size \"native/sampling\"\n");
      exit(-1);
    }

    int size=0;
    int read = sscanf(argv[1], "%d", &size);
    if(read==0){
      if (rank == 0)
        printf("Invalid argument %s\n", argv[1]);
      exit(-1);
    }else{
      if (rank == 0)
        printf("Matrix Size : %dx%d\n",size,size);
    }

    float *a = (float*)malloc(sizeof(float)*size*size);
    float *b = (float*)malloc(sizeof(float)*size*size);
    float *c = (float*)malloc(sizeof(float)*size*size);

    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    if (rank == 0) {
        // Initialize buffers.
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                a[i*size+j] = (float)i + j;
                b[i*size+j] = (float)i - j;
                c[i*size+j] = 0.0f;
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
    if (strcmp(argv[2], "sampling")){
      if (rank == 0)
        printf ("Native mode\n");
      multiply(a, b, c, istart, iend, size);
    }else{
      if (rank == 0)
        printf ("Sampling mode\n");
      multiply_sampled(a, b, c, istart, iend, size);
    }

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
            if (strcmp(argv[2], "sampling"))
                multiply(a, b, c, (size/nproc)*nproc, size-1, size);
	    else
                multiply_sampled(a, b, c, (size/nproc)*nproc, size-1, size);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    MPI_Finalize();
    free(a);
    free(b);
    free(c);
    if (rank == 0) { /* use time on master node */
        //float msec_total = 0.0f;

        // Compute and print the performance
        float sec_per_matrix_mul = end-start;
        double flops_per_matrix_mul = 2.0 * (double)size * (double)size * (double)size;
        double giga_flops = (flops_per_matrix_mul * 1.0e-9f) / (sec_per_matrix_mul / 1000.0f);
        printf(
            "Performance= %.2f GFlop/s, Time= %.3f sec, Size= %.0f Ops\n",
            giga_flops,
            sec_per_matrix_mul,
            flops_per_matrix_mul);
    }

    return 0;
}
