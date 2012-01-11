/* A simple example pingpong pogram to test MPI_Send and MPI_Recv */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank;
    int size;
    MPI_Status status;

    int n = 0, m = 0, bytes = 0, workusecs = 0;
    int currusecs;
  
    char *buf = NULL;
    int i, j;
    struct timeval start_time, stop_time;
    struct timeval start_pause, curr_pause;
    unsigned long usecs;

    MPI_Init(&argc, &argv); /* Initialize MPI */
    MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */

    if (size != 2) {
        printf("run this program with exactly 2 processes (-np 2)\n");
        MPI_Finalize();
        exit(0);
    }

    if (0 == rank) {
        if (argc > 1 && isdigit(argv[1][0])) {
            n = atoi(argv[1]);
        }
        if (argc > 2 && isdigit(argv[2][0])) {
            m = atoi(argv[2]);
        }
        if (argc > 3 && isdigit(argv[3][0])) {
            bytes = atoi(argv[3]);
        }
        if (argc > 4 && isdigit(argv[4][0])) {
            workusecs = atoi(argv[4]);
        }
        buf = malloc(sizeof(char) * bytes);
        for (i = 0; i < bytes; i++) buf[i] = i % 256;
        MPI_Send(&n, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&m, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&bytes, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&workusecs, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        gettimeofday(&start_time, NULL);
        for (i = 0; i < n; i++) {
            for (j = 0; j < m; j++) {
                MPI_Send(buf, bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                gettimeofday(&start_pause, NULL);
                currusecs = 0;
                while (currusecs < workusecs) {
                    gettimeofday(&curr_pause, NULL);
                    currusecs = (curr_pause.tv_sec - start_pause.tv_sec) * 1e6 + curr_pause.tv_usec - start_pause.tv_usec;
                }
            }
            MPI_Recv(buf, bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
        }
        gettimeofday(&stop_time, NULL);
        usecs = (stop_time.tv_sec - start_time.tv_sec) * 1e6 + stop_time.tv_usec - start_time.tv_usec;
        printf("n: %d m: %d bytes: %d sleep: %d usecs: %u\n", n, m, bytes, workusecs, usecs);
    } else {
        MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&bytes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&workusecs, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        buf = malloc(sizeof(char) * bytes);
        MPI_Barrier(MPI_COMM_WORLD);
        for (i = 0; i < n; i++) {
            for (j = 0; j < m; j++) {
                MPI_Recv(buf, bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
            }
            MPI_Send(buf, bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
    }
    free(buf);
    MPI_Finalize();
    return 0;
}
