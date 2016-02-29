/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "mpi.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int master(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long slaves_count = xbt_str_parse_int(argv[4], "Invalid amount of slaves: %s");

  int i;

  XBT_INFO("Got %ld slaves and %ld tasks to process", slaves_count, number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    char sprintf_buffer[256];
    msg_task_t task = NULL;

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    sprintf(sprintf_buffer, "Task_%d", i);
    task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    if (number_of_tasks < 10000 || i % 10000 == 0)
      XBT_INFO("Sending \"%s\" (of %ld) to mailbox \"%s\"", task->name, number_of_tasks, mailbox);

    MSG_task_send(task, mailbox);
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    char mailbox[80];

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox);
  }

  return 0;
}

static int master_mpi(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  XBT_INFO("here for rank %d", rank);
  int test[1000]={rank};
  if(rank==0)
    MPI_Send(&test, 1000, MPI_INT, 1, 1, MPI_COMM_WORLD);
  else
    MPI_Recv(&test, 1000, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

  XBT_INFO("After comm %d", rank);
  MPI_Finalize();

  XBT_INFO("After finalize %d %d", rank, test[0]);
  return 0;
}

static int alltoall_mpi(int argc, char *argv[])
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
}

static int slave(int argc, char *argv[])
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
}

int main(int argc, char *argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);

  xbt_assert(argc > 2,"Usage: %s platform_file deployment_file\n"
             "\nexample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  // launch two MPI applications as well, one using master_mpi function as main on 2 nodes
  SMPI_app_instance_register("master_mpi", master_mpi,2);
  // the second performing an alltoall on 4 nodes
  SMPI_app_instance_register("alltoall_mpi", alltoall_mpi,4);
  MSG_launch_application(argv[2]);
  SMPI_init();

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  SMPI_finalize();
  return res != MSG_OK;
}
