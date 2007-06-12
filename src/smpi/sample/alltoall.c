#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

// sandler says, compile with mpicc -v alltoalldemo.c
// run with mpirun -np 3 a.out -m 5

int main( int argc, char *argv[] )
{
    int rank, size;
    int chunk = 128;
    int i;
     int j; // added by sandler
    int *sb;
    int *rb;
    int status, gstatus;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
     if (rank==0) {
        printf("size: %d\n", size);
    }
    for ( i=1 ; i < argc ; ++i ) {
        if ( argv[i][0] != '-' ) {
                // added by sandler
                fprintf(stderr, "Unrecognized option %s\n", argv[i]);fflush(stderr);
            continue;
            }
        switch(argv[i][1]) {
            case 'm':
                chunk = atoi(argv[++i]);
                     if (rank==0) {
                        printf("chunk: %d\n", chunk);
                    }
                break;
            default:
                fprintf(stderr, "Unrecognized argument %s\n", argv[i]);fflush(stderr);
                MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
        }
    }
    sb = (int *)malloc(size*chunk*sizeof(int));
    if ( !sb ) {
        perror( "can't allocate send buffer" );fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
    }
    rb = (int *)malloc(size*chunk*sizeof(int));
    if ( !rb ) {
        perror( "can't allocate recv buffer");fflush(stderr);
        free(sb);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
     
     
     /* original deino.net:
    for ( i=0 ; i < size*chunk ; ++i ) {
        sb[i] = sb[i] = rank + 1;
        rb[i] = 0;
    }
    */
        // written by sandler
        
        if (rank==0) printf("note in the following:\n"
        "if you were to compare the sending buffer and the receiving buffer on the SAME processor, \n"
        "you might think that the values were getting wiped out.  However, each row IS going somewhere. \n"
        "The 0th row of processor 0 goes to the 0th row of processor 0\n"
        "The 1st row of processor 0 goes to the 0th row of processor 1.  (Go look at rb for processor 1!)\n"
        "\n"
        "Too bad the values don't come out in a deterministic order. That's life!\n"
        "\n"
        "Now look at the receiving buffer for processor 0.\n"
        "The 0th row is from processor 0 (itself).\n"
        "The 1st row on processor 0 is from the 0th row on processor 1. (Go look at the sb of processor 1!)\n"
        "\n"
        "Apparently this is the intended behavior.\n"
        "\n"
        "Note that each row is always moved as one chunk, unchangeable.\n"
        "\n"
        "TODO: draw a diagram\n"
        );
        
        for (i=0; i<size; i++) {
            for (j=0; j<chunk; j++) {
                int offset = i*chunk + j; // note the multiplier is chunk, not size

                sb[offset] = rank*100 + i*10 + j;
                rb[offset] = -1;
            }
        }
    
    
        // this clearly shows what is NOT indended to be done, in that the rb on a processor is the same as the sb on the processor 
        // in this intialization: on processor 0, only the 0th row gets normal values.
        // on processor 1, only the 1st row gets normal values.
        // when you look the rb, it looks like nothing happened.  this is because, say, for processor 1, the 1st row got sent to itself.
        /*
        for (i=0; i<size; i++) {
            for (j=0; j<chunk; j++) {
                int offset = i*chunk + j; // note the multiplier is chunk, not size
                
                if (i==rank) 
                    sb[offset] = rank*100 + i*10 + j;
                else 
                    sb[offset] = 999;
                    
                rb[i*chunk + j] = 999;
            }
        }
    */
        
        // this does printgrid("sb", rank, size, chunk, sb);
        //added by sandler
        printf("[processor %d] To send:\n", rank);
        for (i=0; i<size; i++) {
            for (j=0; j<chunk; j++) {
                // note the multiplier is chunk, not size
                printf("%03d ", sb[i*chunk+j]);       
            }
            printf("\n");
        }
        printf("\n");

/*
        // for another variation, could send out a bunch of characters, like
        p r o c e s s o r 0 r o w 0
        p r o c e s s o r 0 r o w 1
        p r o c e s s o r 0 r o w 2
        
        then you'd get back
        
        p r o c e s s o r 0 r o w 0
        p r o c e s s o r 1 r o w 0
        p r o c e s s o r 2 r o w 0
*/      
        

    status = MPI_Alltoall(sb, chunk, MPI_INT, rb, chunk, MPI_INT, MPI_COMM_WORLD);

        // this does printgrid("rb", rank, size, chunk, rb);
        // added by sandler
        printf("[processor %d] Received:\n", rank);
        for (i=0; i<size; i++) {
            for (j=0; j<chunk; j++) {
                printf("%03d ", rb[i*chunk+j]);       
            }
            printf("\n");
        }
        printf("\n");


    MPI_Allreduce( &status, &gstatus, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
        if (gstatus != 0) {
            printf("all_to_all returned %d\n",gstatus);fflush(stdout);
        }

        
   }



    free(sb);
    free(rb);
    MPI_Finalize();
    return(EXIT_SUCCESS);
}
