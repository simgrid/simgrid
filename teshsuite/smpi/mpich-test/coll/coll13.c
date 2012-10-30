#include "mpi.h"
#include "test.h"

/* 
From: hook@nas.nasa.gov (Edward C. Hook)
 */

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int main( int argc, char *argv[] )
{
    int rank, size;
    int chunk = 4096;
    int i,j;
    int *sb;
    int *rb;
    int status, gstatus, endstatus;
    endstatus=0;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    for ( i=1 ; i < argc ; ++i ) {
	if ( argv[i][0] != '-' )
	    continue;
	switch(argv[i][1]) {
	case 'm':
	    chunk = atoi(argv[++i]);
	    break;
	default:
	    fprintf(stderr,"Unrecognized argument %s\n",
		    argv[i]);
	    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
	}
    }
     
    
    /*
    SMPI addition : we want to test all three alltoall algorithms, thus we use three diffrent sizes
    this is the code that handles these cases
    if (sendsize < 200 && size > 12) {
      retval =
          smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype,
                                         comm);
    } else if (sendsize < 3000) {
      retval =
          smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount,
                                                sendtype, recvbuf,
                                                recvcount, recvtype, comm);
    } else {
      retval =
          smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype,
                                            comm);
    }
    
    
    */
    
    
    int sizes [3] ={ 4096, 64, 32};
    for ( j=0 ; j < 3 ; j++ ) {
      sb = (int *)malloc(size*sizes[j]*sizeof(int));
      if ( !sb ) {
        perror( "can't allocate send buffer" );
        MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
      }
      rb = (int *)malloc(size*sizes[j]*sizeof(int));
      if ( !rb ) {
        perror( "can't allocate recv buffer");
        free(sb);
        MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
      }
      for ( i=0 ; i < size*sizes[j] ; ++i ) {
        sb[i] = rank + 1;
        rb[i] = 0;
      }

      /* fputs("Before MPI_Alltoall\n",stdout); */
      MPI_Barrier(MPI_COMM_WORLD );
      /* This should really send MPI_CHAR, but since sb and rb were allocated
       as chunk*size*sizeof(int), the buffers are large enough */
      status = MPI_Alltoall(sb,sizes[j],MPI_INT,rb,sizes[j],MPI_INT,
                            MPI_COMM_WORLD);

      /* fputs("Before MPI_Allreduce\n",stdout); */
      MPI_Allreduce( &status, &gstatus, 1, MPI_INT, MPI_SUM, 
                    MPI_COMM_WORLD );

      MPI_Barrier(MPI_COMM_WORLD );
    /* fputs("After MPI_Allreduce\n",stdout); */
      if (rank == 0 && gstatus != 0) endstatus ++;

      free(sb);
      free(rb);
    }
    
    if (rank == 0) {
      if (endstatus == 0) printf( " No Errors\n" );
      else 
        printf("all_to_all returned %d erros\n",endstatus);
    }
    MPI_Finalize();

    return(EXIT_SUCCESS);
}

