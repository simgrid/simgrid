#include "mpi.h"

double start[64], elapsed[64];

void timer_clear( int n )
{
    elapsed[n] = 0.0;
}

void timer_start( int n )
{
    start[n] = MPI_Wtime();
}

void timer_stop( int n )
{
    double t, now;
    now = MPI_Wtime();
    t = now - start[n];
    elapsed[n] += t;
}

double timer_read( int n )
{
    return( elapsed[n] );
}

