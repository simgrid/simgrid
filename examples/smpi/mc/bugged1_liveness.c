/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* Bug : CS requests of process 1 not satisfied                                      */
/* LTL property checked : G(r->F(cs)); (r=request of CS, cs=CS ok)            */
/******************************************************************************/

/* Run :
  /usr/bin/time -f "clock:%e user:%U sys:%S swapped:%W exitval:%x max:%Mk" "$@" ../../../smpi_script/bin/smpirun -hostfile hostfile_bugged1_liveness -platform ../../platforms/cluster.xml --cfg=contexts/factory:ucontext --cfg=model-check/reduction:none --cfg=model-check/property:promela_bugged1_liveness --cfg=smpi/send_is_detached_thresh:0 --cfg=contexts/stack_size:128 --cfg=model-check/visited:100000 --cfg=model-check/max_depth:100000 ./bugged1_liveness */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

#define GRANT_TAG 0
#define REQUEST_TAG 1
#define RELEASE_TAG 2

int r, cs;

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

  MC_automaton_new_propositional_symbol_pointer("r", &r);
  MC_automaton_new_propositional_symbol_pointer("cs", &cs);

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
          printf("CS already used.\n");
          xbt_dynar_push(requests, &recv_buff);
        }else{
          if(recv_buff != size - 1){
            printf("CS idle. Grant immediatly.\n");
            MPI_Send(&rank, 1, MPI_INT, recv_buff, GRANT_TAG, MPI_COMM_WORLD);
            CS_used = 1;
          }
        }
      }else{
        if(!xbt_dynar_is_empty(requests)){
          printf("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
          xbt_dynar_shift(requests, &recv_buff);
          if(recv_buff != size - 1){
            MPI_Send(&rank, 1, MPI_INT, recv_buff, GRANT_TAG, MPI_COMM_WORLD);
            CS_used = 1;
          }else{
            xbt_dynar_push(requests, &recv_buff);
            CS_used = 0;
          }
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
      if(rank == size - 1){
        r = 1;
        cs = 0;
      }
      MPI_Recv(&recv_buff, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG == GRANT_TAG && rank == size - 1){
        cs = 1;
        r = 0;
      }
      printf("%d got the answer. Release it.\n", rank);
      MPI_Send(&rank, 1, MPI_INT, 0, RELEASE_TAG, MPI_COMM_WORLD);
      if(rank == size - 1){
        r = 0;
        cs = 0;
      }
    }
  }

  MPI_Finalize();

  return 0;
}
