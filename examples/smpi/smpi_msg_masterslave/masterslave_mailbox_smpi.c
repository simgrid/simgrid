/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */
#include "mpi.h"
/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int master_mpi(int argc, char *argv[]);
int alltoall_mpi(int argc, char *argv[]);

/** Emitter function  */
int master(int argc, char *argv[])
{
  long number_of_tasks = atol(argv[1]);
  double task_comp_size = atof(argv[2]);
  double task_comm_size = atof(argv[3]);
  long slaves_count = atol(argv[4]);

  int i;

  XBT_INFO("Got %ld slaves and %ld tasks to process", slaves_count,
        number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    char sprintf_buffer[256];
    msg_task_t task = NULL;

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    sprintf(sprintf_buffer, "Task_%d", i);
    task =
        MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                        NULL);
    if (number_of_tasks < 10000 || i % 10000 == 0)
      XBT_INFO("Sending \"%s\" (of %ld) to mailbox \"%s\"", task->name,
            number_of_tasks, mailbox);

    MSG_task_send(task, mailbox);
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    char mailbox[80];

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox);
  }

//  XBT_INFO("Goodbye now!");
  return 0;
}                               /* end_of_master */


int master_mpi(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  XBT_INFO("here for rank %d", rank);
  int test[1000]={rank};
  if(rank==0)
        MPI_Send(&test, 1000, 
                 MPI_INT, 1, 1, MPI_COMM_WORLD); 
  else
        MPI_Recv(&test, 1000, 
                 MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUSES_IGNORE); 

  XBT_INFO("After comm %d", rank);
  MPI_Finalize();

  XBT_INFO("After finalize %d %d", rank, test[0]);
  return 0;
}                               /* end_of_master */



int alltoall_mpi(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  XBT_INFO("alltoall for rank %d", rank);
  int* out=malloc(1000*size*sizeof(int));
  int* in=malloc(1000*size*sizeof(int));
  MPI_Alltoall(out, 1000, MPI_INT,in, 1000, MPI_INT, MPI_COMM_WORLD); 

  XBT_INFO("after alltoall %d", rank);
  free(out);
  free(in);
  MPI_Finalize();
  return 0;
}                               /* end_of_master */


/** Receiver function  */
int slave(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED int res;
  int id = -1;
  char mailbox[80];
  XBT_ATTRIB_UNUSED int read;

  read = sscanf(argv[1], "%d", &id);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);

  sprintf(mailbox, "slave-%d", id);

  while (1) {
    res = MSG_task_receive(&(task), mailbox);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

//  XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }
//    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
//    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  XBT_INFO("I'm done. See you!");

  return 0;
}                               /* end_of_slave */

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res;
  const char *platform_file;
  const char *application_file;

  MSG_init(&argc, argv);

  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  platform_file = argv[1];
  application_file = argv[2];

  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    // launch two MPI applications as well, one using master_mpi function as main on 2 nodes
    SMPI_app_instance_register("master_mpi", master_mpi,2);
    // the second performing an alltoall on 4 nodes
    SMPI_app_instance_register("alltoall_mpi", alltoall_mpi,4);
    MSG_launch_application(application_file);
    SMPI_init();
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());


  SMPI_finalize();
  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
