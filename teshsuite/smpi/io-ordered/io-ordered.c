/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

/* Test reading and writing zero bytes (set status correctly) */

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size;
    int rank;
    int* buf;
    int count;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Status status;

    MPI_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;
    MPI_File_open( comm, (char*)"/scratch/testfile", MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh );
    MPI_Comm_size( comm, &size );
    MPI_Comm_rank( comm, &rank );
    buf = (int *)malloc( 10* sizeof(int) );
    buf[0] = rank;

    /* Write to file */
    MPI_File_write_ordered( fh, buf, 10, MPI_INT, &status );
    MPI_Get_count( &status, MPI_INT, &count );
    if (count != 10) {
        errs++;
        fprintf( stderr, "Wrong count (%d) on write-ordered\n", count );fflush(stderr);
    }
    MPI_Barrier( comm );
    MPI_File_seek_shared( fh, 0, MPI_SEEK_SET );
    for (int i = 0; i < size; i++)
      buf[i] = -1;
    MPI_File_read_ordered( fh, buf, 10, MPI_INT, &status );

    MPI_Barrier(comm);
    MPI_File_seek_shared( fh, 0, MPI_SEEK_SET );

    free( buf );
    MPI_File_close( &fh );

    MPI_Finalize();
    return errs;
}
