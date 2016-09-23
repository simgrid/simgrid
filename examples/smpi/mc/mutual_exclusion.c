/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

#define GRANT_TAG 0
#define REQUEST_TAG 1
#define RELEASE_TAG 2

int main(int argc, char **argv){
  int err, size, rank;
  int recv_buff;
  MPI_Status status;
  int CS_used = 0;
  xbt_dynar_t requests = xbt_dynar_new(sizeof(int), NULL);

  /* Initialize MPI */
  err = MPI_Init(&argc, &argv);
  if(err !=  MPI_SUCCESS){
    printf("MPI initialization failed !\n");
    exit(1);
  }

  MC_ignore(&(status.count), sizeof(status.count));

  /* Get number of processes */
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  /* Get id of this process */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if(rank == 0){ /* Coordinator */
    while(1){
      MPI_Recv(&recv_buff, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG == REQUEST_TAG){
        if(CS_used){
          printf("CS already used. Queue the request.\n");
          xbt_dynar_push(requests, &recv_buff);
        }else{
          printf("CS idle. Grant immediatly.\n");
          MPI_Send(&rank, 1, MPI_INT, recv_buff, GRANT_TAG, MPI_COMM_WORLD);
          CS_used = 1;
        }
      }else{
        if(!xbt_dynar_is_empty(requests)){
          printf("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
          xbt_dynar_shift(requests, &recv_buff);
          MPI_Send(&rank, 1, MPI_INT, recv_buff, GRANT_TAG, MPI_COMM_WORLD);
          CS_used = 1;
        }else{
          printf("CS release. Resource now idle.\n");
          CS_used = 0;
        }
      }
    }
  }else{ /* Client */
    while(1){
      printf("%d asks the request.\n", rank);
      MPI_Send(&rank, 1, MPI_INT, 0, REQUEST_TAG, MPI_COMM_WORLD);

      MPI_Recv(&recv_buff, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      printf("%d got the answer. Release it.\n", rank);
      MPI_Send(&rank, 1, MPI_INT, 0, RELEASE_TAG, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();

  return 0;
}
