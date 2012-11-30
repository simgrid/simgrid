#include <stdio.h>
#include "mpi.h"
#define SIZE 4

int main(int argc, char **argv) {

  int rank, i, j;
  double a[SIZE][SIZE] = {{0}};

  MPI_Datatype columntype;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Type_hvector(SIZE, 1, SIZE*sizeof(double), MPI_DOUBLE, &columntype);
  MPI_Type_commit(&columntype);


    if (rank == 0) {
      for(i=0; i <SIZE;i++)
        for(j=0; j <SIZE;j++)
          a[i][j] = i*SIZE+j;
    }

    /* only one column is send
     * this is an exemple for non-contignous data*/
    MPI_Bcast(a, 1, columntype, 0, MPI_COMM_WORLD);

    for(i=0; i<SIZE; i++){
      for (j=0; j < SIZE; j++) {
        printf("rank= %d, a[%d][%d]=%f\n",
               rank, i, j, a[i][j]);
      }
      printf("\n");
    }



  MPI_Finalize();
  return 0;
}

