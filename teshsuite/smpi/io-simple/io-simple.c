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
    int i;
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
    buf = (int *)malloc( size * sizeof(int) );
    buf[0] = rank;
 
    /* Write to file */
    MPI_File_seek( fh, sizeof(int)*rank, MPI_SEEK_SET ); 
    MPI_File_write( fh, buf, 1, MPI_INT, &status );
    MPI_Get_count( &status, MPI_INT, &count );
    if (count != 1) {
        errs++;
        fprintf( stderr, "Wrong count (%d) on write\n", count );fflush(stderr);
    }

    /* Read nothing (check status) */
    memset( &status, 0xff, sizeof(MPI_Status) );
    MPI_File_read( fh, buf, 0, MPI_INT, &status );
    MPI_Get_count( &status, MPI_INT, &count );
    if (count != 0) {
        errs++;
        fprintf( stderr, "Count not zero (%d) on read\n", count );fflush(stderr);
    }

    /* Write nothing (check status) */
    memset( &status, 0xff, sizeof(MPI_Status) );
    MPI_File_write( fh, buf, 0, MPI_INT, &status );
    if (count != 0) {
        errs++;
        fprintf( stderr, "Count not zero (%d) on write\n", count );fflush(stderr);
    }

    MPI_Barrier( comm );

    MPI_File_seek( fh, sizeof(int)*rank, MPI_SEEK_SET );
    for (i=0; i<size; i++) buf[i] = -1;
    MPI_File_read( fh, buf, 1, MPI_INT, &status );
    // if (buf[0] != rank) {
        // errs++;
        // fprintf( stderr, "%d: buf = %d\n", rank, buf[0] );fflush(stderr);
    // }
 
    free( buf );
    MPI_File_close( &fh );
 
    MPI_Finalize();
    return errs;
}
