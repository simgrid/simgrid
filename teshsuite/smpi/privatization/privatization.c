
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


static int myvalue = 0;

int main(int argc, char **argv)
{
    int me;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    MPI_Barrier(MPI_COMM_WORLD);

    myvalue = me;

    MPI_Barrier(MPI_COMM_WORLD);

    if(myvalue!=me)
      printf("Privatization error - %d != %d\n", myvalue, me);
    MPI_Finalize();
    return 0;
}
