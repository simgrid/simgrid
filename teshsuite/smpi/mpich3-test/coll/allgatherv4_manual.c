/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#include <math.h>
#include <assert.h>

/* FIXME: What is this test supposed to accomplish? */

#define START_BUF (1)
#define LARGE_BUF (256 * 1024)

/* FIXME: MAX_BUF is too large */
#define MAX_BUF   (32 * 1024 * 1024)
#define LOOPS 10

SMPI_VARINIT_GLOBAL(sbuf, char*);
SMPI_VARINIT_GLOBAL(rbuf, char*);
SMPI_VARINIT_GLOBAL(recvcounts, int*);
SMPI_VARINIT_GLOBAL(displs, int*);
SMPI_VARINIT_GLOBAL_AND_SET(errs, int, 0);

/* #define dprintf printf */
#define dprintf(...)

typedef enum {
    REGULAR,
    BCAST,
    SPIKE,
    HALF_FULL,
    LINEAR_DECREASE,
    BELL_CURVE
} test_t;

void comm_tests(MPI_Comm comm);
double run_test(long long msg_size, MPI_Comm comm, test_t test_type, double * max_time);

int main(int argc, char ** argv)
{
    int comm_size, comm_rank;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

    if (LARGE_BUF * comm_size > MAX_BUF)
        goto fn_exit;

    SMPI_VARGET_GLOBAL(sbuf) = (void *) calloc(MAX_BUF, 1);
    SMPI_VARGET_GLOBAL(rbuf) = (void *) calloc(MAX_BUF, 1);

    srand(time(NULL));

    SMPI_VARGET_GLOBAL(recvcounts) = (void *) malloc(comm_size * sizeof(int));
    SMPI_VARGET_GLOBAL(displs) = (void *) malloc(comm_size * sizeof(int));
    if (!SMPI_VARGET_GLOBAL(recvcounts) || !SMPI_VARGET_GLOBAL(displs) || !SMPI_VARGET_GLOBAL(sbuf) || !SMPI_VARGET_GLOBAL(rbuf)) {
        fprintf(stderr, "Unable to allocate memory:\n");
	if (!SMPI_VARGET_GLOBAL(sbuf))
            fprintf(stderr,"\tsbuf of %d bytes\n", MAX_BUF );
	if (!SMPI_VARGET_GLOBAL(rbuf))
            fprintf(stderr,"\trbuf of %d bytes\n", MAX_BUF );
        if (!SMPI_VARGET_GLOBAL(recvcounts))
            fprintf(stderr,"\trecvcounts of %zu bytes\n", comm_size * sizeof(int));
        if (!SMPI_VARGET_GLOBAL(displs))
            fprintf(stderr,"\tdispls of %zu bytes\n", comm_size * sizeof(int));
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(-1);
    }

    if (!comm_rank) {
        dprintf("Message Range: (%d, %d); System size: %d\n", START_BUF, LARGE_BUF, comm_size);
        fflush(stdout);
    }


    /* COMM_WORLD tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         MPI_COMM_WORLD\n");
        dprintf("==========================================================\n");
    }
    comm_tests(MPI_COMM_WORLD);

    /* non-COMM_WORLD tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         non-COMM_WORLD\n");
        dprintf("==========================================================\n");
    }
    MPI_Comm_split(MPI_COMM_WORLD, (comm_rank == comm_size - 1) ? 0 : 1, 0, &comm);
    if (comm_rank < comm_size - 1)
        comm_tests(comm);
    MPI_Comm_free(&comm);

    /* Randomized communicator tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         Randomized Communicator\n");
        dprintf("==========================================================\n");
    }
    MPI_Comm_split(MPI_COMM_WORLD, 0, rand(), &comm);
    comm_tests(comm);
    MPI_Comm_free(&comm);

    //free(SMPI_VARGET_GLOBAL(sbuf));
    //free(SMPI_VARGET_GLOBAL(rbuf));
    free(SMPI_VARGET_GLOBAL(recvcounts));
    free(SMPI_VARGET_GLOBAL(displs));

fn_exit:
    MTest_Finalize(SMPI_VARGET_GLOBAL(errs));
    MPI_Finalize();

    return 0;
}

void comm_tests(MPI_Comm comm)
{
    int comm_size, comm_rank;
    double rtime = rtime;       /* stop warning about unused variable */
    double max_time;
    long long msg_size;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    for (msg_size = START_BUF; msg_size <= LARGE_BUF; msg_size *= 2) {
        if (!comm_rank) {
            dprintf("\n====> MSG_SIZE: %d\n", (int) msg_size);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, REGULAR, &max_time);
        if (!comm_rank) {
            dprintf("REGULAR:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, BCAST, &max_time);
        if (!comm_rank) {
            dprintf("BCAST:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, SPIKE, &max_time);
        if (!comm_rank) {
            dprintf("SPIKE:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, HALF_FULL, &max_time);
        if (!comm_rank) {
            dprintf("HALF_FULL:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, LINEAR_DECREASE, &max_time);
        if (!comm_rank) {
            dprintf("LINEAR_DECREASE:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }

        rtime = run_test(msg_size, comm, BELL_CURVE, &max_time);
        if (!comm_rank) {
            dprintf("BELL_CURVE:\tAVG: %.3f\tMAX: %.3f\n", rtime, max_time);
            fflush(stdout);
        }
    }
}

double run_test(long long msg_size, MPI_Comm comm, test_t test_type,
		double * max_time)
{
    int i, j;
    int comm_size, comm_rank;
    double start, end;
    double total_time, avg_time;
    MPI_Aint tmp;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    SMPI_VARGET_GLOBAL(displs)[0] = 0;
    for (i = 0; i < comm_size; i++) {
        if (test_type == REGULAR)
            SMPI_VARGET_GLOBAL(recvcounts)[i] = msg_size;
        else if (test_type == BCAST)
            SMPI_VARGET_GLOBAL(recvcounts)[i] = (!i) ? msg_size : 0;
        else if (test_type == SPIKE)
            SMPI_VARGET_GLOBAL(recvcounts)[i] = (!i) ? (msg_size / 2) : (msg_size / (2 * (comm_size - 1)));
        else if (test_type == HALF_FULL)
            SMPI_VARGET_GLOBAL(recvcounts)[i] = (i < (comm_size / 2)) ? (2 * msg_size) : 0;
        else if (test_type == LINEAR_DECREASE) {
            tmp = 2 * msg_size * (comm_size - 1 - i) / (comm_size - 1);
	    if (tmp != (int)tmp) {
		fprintf( stderr, "Integer overflow in variable tmp\n" );
		MPI_Abort( MPI_COMM_WORLD, 1 );
                exit(1);
	    }
            SMPI_VARGET_GLOBAL(recvcounts)[i] = (int) tmp;

            /* If the maximum message size is too large, don't run */
            if (tmp > MAX_BUF) return 0;
        }
        else if (test_type == BELL_CURVE) {
            for (j = 0; j < i; j++) {
                if (i - 1 + j >= comm_size) continue;
                tmp = msg_size * comm_size / (log(comm_size) * i);
                SMPI_VARGET_GLOBAL(recvcounts)[i - 1 + j] = (int) tmp;
                SMPI_VARGET_GLOBAL(displs)[i - 1 + j] = 0;

                /* If the maximum message size is too large, don't run */
                if (tmp > MAX_BUF) return 0;
            }
        }

        if (i < comm_size - 1)
            SMPI_VARGET_GLOBAL(displs)[i+1] = SMPI_VARGET_GLOBAL(displs)[i] + SMPI_VARGET_GLOBAL(recvcounts)[i];
    }

    /* Test that:
       1: sbuf is large enough
       2: rbuf is large enough
       3: There were no failures (e.g., tmp nowhere > rbuf size
    */
    MPI_Barrier(comm);
    start = MPI_Wtime();
    for (i = 0; i < LOOPS; i++) {
        MPI_Allgatherv(SMPI_VARGET_GLOBAL(sbuf), SMPI_VARGET_GLOBAL(recvcounts)[comm_rank], MPI_CHAR,
                       SMPI_VARGET_GLOBAL(rbuf), SMPI_VARGET_GLOBAL(recvcounts), SMPI_VARGET_GLOBAL(displs), MPI_CHAR, comm);
    }
    end = MPI_Wtime();
    MPI_Barrier(comm);

    /* Convert to microseconds (why?) */
    total_time = 1.0e6 * (end - start);
    MPI_Reduce(&total_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    MPI_Reduce(&total_time, max_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

    return (avg_time / (LOOPS * comm_size));
}
