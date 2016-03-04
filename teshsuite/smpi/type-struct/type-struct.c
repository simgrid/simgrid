/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    int          rank;
    struct { int a;int c; double b;int tab[2][3];} value;
    MPI_Datatype mystruct;
    int          blocklens[3];
    MPI_Aint     indices[3];
    MPI_Datatype old_types[3], type2;
    int i,j;

    MPI_Init( &argc, &argv );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int tab[2][3]={{1*rank,2*rank,3*rank},{7*rank,8*rank,9*rank}}; 
    MPI_Type_contiguous(3, MPI_INT, &type2);
    MPI_Type_commit(&type2);

    /* One value of each type, and two for the contiguous one */
    blocklens[0] = 1;
    blocklens[1] = 1;
    blocklens[2] = 2;
    /* The base types */
    old_types[0] = MPI_INT;
    old_types[1] = MPI_DOUBLE;
    old_types[2] = type2;
    /* The locations of each element */
    MPI_Address( &value.a, &indices[0] );
    MPI_Address( &value.b, &indices[1] );
    MPI_Address( &tab, &indices[2] );
    /* Make relative */
    indices[2] = indices[2] - indices[0];
    indices[1] = indices[1] - indices[0];
    indices[0] = 0;

    MPI_Type_struct( 3, blocklens, indices, old_types, &mystruct );
    MPI_Type_commit( &mystruct );

    if (rank == 0){
      value.a=-2;
      value.b=8.0;
    }else{
      value.a=10000;
      value.b=5.0; 
    }

    MPI_Bcast( &value, 1, mystruct, 0, MPI_COMM_WORLD );

    printf( "Process %d got %d (-2?) and %f (8.0?), tab (should be all 0): ", rank, value.a, value.b );

    for(j=0; j<2;j++ )
      for(i=0; i<3;i++ )
        printf("%d ", tab[j][i]);
    printf("\n");

    /* Clean up the type */
    MPI_Type_free( &mystruct );
    MPI_Type_free( &type2 );
    MPI_Finalize( );
    return 0;
}
