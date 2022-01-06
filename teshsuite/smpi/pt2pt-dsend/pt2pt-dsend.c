/* Copyright (c) 2011-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This program simply does a very small exchange to test whether using SIMIX dsend to model the eager mode works */
#include <stdint.h>
#include <stdio.h>
#include <mpi.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(dsend,"the dsend test");

static void test_opts(int argc, char* const argv[])
{
  int found = 0;
  int option_index = 0;
  static struct option long_options[] = {
  {(char*)"long",     no_argument, 0,  0 },
  {0,         0,                 0,  0 }
  };
  while (1) {
    int ret = getopt_long(argc, argv, "s", long_options, &option_index);
    if(ret==-1)
      break;

    switch (ret) {
      case 0:
      case 's':
        found ++;
      break;
      default:
        printf("option %s", long_options[option_index].name);
      break;
    }
  }
  if (found!=2){
    printf("(smpi_)getopt_long failed ! \n");
  }
}

int main(int argc, char *argv[])
{
  int rank;
  int32_t data=11;

  MPI_Init(NULL, NULL);

  /* test getopt_long function */
  test_opts(argc, argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Request r;
  if (rank==1) {
    data=22;
    MPI_Send(&data,1,MPI_INT32_T,(rank+1)%2,666,MPI_COMM_WORLD);
  } else {
    MPI_Recv(&data,1,MPI_INT32_T,MPI_ANY_SOURCE,666,MPI_COMM_WORLD,NULL);
    if (data !=22) {
      printf("rank %d: Damn, data does not match (got %d)\n",rank, data);
    }
  }

  if (rank==1) {
    data=22;
    MPI_Isend(&data,1,MPI_INT32_T,(rank+1)%2,666,MPI_COMM_WORLD, &r);
    MPI_Wait(&r, MPI_STATUS_IGNORE);
  } else {
    MPI_Irecv(&data,1,MPI_INT32_T,MPI_ANY_SOURCE,666,MPI_COMM_WORLD,&r);
    MPI_Wait(&r, MPI_STATUS_IGNORE);
    if (data !=22) {
      printf("rank %d: Damn, data does not match (got %d)\n",rank, data);
    }
  }

  XBT_INFO("rank %d: data exchanged", rank);
  MPI_Finalize();
  exit(0);
}
