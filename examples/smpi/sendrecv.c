#include "mpi.h"
#include <stdio.h>
 
int main(int argc, char *argv[])
{
#define TAG_RCV 998
#define TAG_SND 999
    int myid, numprocs, left, right;
    int buffer[10], buffer2[10];
    MPI_Request request;
    MPI_Status status;
 
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
 
    right = (myid + 1) % numprocs;
    left = myid - 1;
    if (left < 0)
        left = numprocs - 1;
 
    MPI_Sendrecv(buffer, 10, MPI_INT, left, TAG_SND, buffer2, 10, MPI_INT, right, TAG_RCV, MPI_COMM_WORLD, &status);
 
    MPI_Finalize();
    return 0;
}
