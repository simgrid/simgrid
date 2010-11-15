/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */
#include <math.h>
/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int sender(int argc, char *argv[]);
int receiver(int argc, char *argv[]);

MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

/** Sender function  */
int sender(int argc, char *argv[])
{
  long number_of_tasks = atol(argv[1]);
  double task_comp_size = atof(argv[2]);
  double task_comm_size = atof(argv[3]);
  long receivers_count = atol(argv[4]);
  int diff_com = atol(argv[5]);
  double coef = 0;
  xbt_dynar_t d = NULL;
  int i;
  m_task_t task = NULL;
  char mailbox[256];
  char sprintf_buffer[256];
  d = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  msg_comm_t *comm = xbt_new(msg_comm_t, number_of_tasks + receivers_count);
  msg_comm_t res_irecv = NULL;
  for (i = 0; i < (number_of_tasks); i++) {
    task = NULL;
    if (diff_com == 0)
      coef = 1;
    else
      coef = (i + 1);

    sprintf(mailbox, "receiver-%ld", (i % receivers_count));
    sprintf(sprintf_buffer, "Task_%d", i);
    task =
        MSG_task_create(sprintf_buffer, task_comp_size,
                        task_comm_size / coef, NULL);
    comm[i] = MSG_task_isend(task, mailbox);
    xbt_dynar_push_as(d, msg_comm_t, comm[i]);
    INFO3("Send to receiver-%ld %s comm_size %f", i % receivers_count,
          sprintf_buffer, task_comm_size / coef);
  }
  /* Here we are waiting for the completion of all communications */

  while (d->used) {
    xbt_dynar_remove_at(d, MSG_comm_waitany(d), &res_irecv);
    MSG_comm_destroy(res_irecv);
  }
  xbt_dynar_free(&d);
  xbt_free(comm);

  /* Here we are waiting for the completion of all tasks */
  sprintf(mailbox, "finalize");

  for (i = 0; i < receivers_count; i++) {
    task = NULL;
    res_irecv = NULL;
    res_irecv = MSG_task_irecv(&(task), mailbox);
    xbt_assert0(MSG_comm_wait(res_irecv, -1) == MSG_OK,
                "MSG_comm_wait failed");
    MSG_task_destroy(task);
  }

  INFO0("Goodbye now!");
  return 0;
}                               /* end_of_sender */

/** Receiver function  */
int receiver(int argc, char *argv[])
{
  int id = -1;
  int i;
  char mailbox[80];
  xbt_dynar_t comms = NULL;
  int tasks = atof(argv[2]);
  comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);

  xbt_assert1(sscanf(argv[1], "%d", &id),
              "Invalid argument %s\n", argv[1]);
  sprintf(mailbox, "receiver-%d", id);
  MSG_process_sleep(10);
  msg_comm_t res_irecv = NULL;
  m_task_t task = NULL;
  m_task_t task_com = NULL;
  for (i = 0; i < tasks; i++) {
    INFO1("Wait to receive task %d", i);
    res_irecv = MSG_task_irecv(&task, mailbox);
    xbt_dynar_push_as(comms, msg_comm_t, res_irecv);
    task = NULL;
    res_irecv = NULL;
  }

  /* Here we are waiting for the receiving of all communications */
  while (comms->used) {
    task_com = NULL;
    res_irecv = NULL;
    MSG_error_t err = MSG_OK;
    int num = MSG_comm_waitany(comms);
    xbt_dynar_remove_at(comms, num, &res_irecv);
    task_com = MSG_comm_get_task(res_irecv);
    MSG_comm_destroy(res_irecv);
    INFO1("Processing \"%s\"", MSG_task_get_name(task_com));
    MSG_task_execute(task_com);
    INFO1("\"%s\" done", MSG_task_get_name(task_com));
    err = MSG_task_destroy(task_com);
    xbt_assert0(err == MSG_OK, "MSG_task_destroy failed");
  }
  xbt_dynar_free(&comms);

  /* Here we tell to sender that all tasks are done */
  sprintf(mailbox, "finalize");
  res_irecv = MSG_task_isend(MSG_task_create(NULL, 0, 0, NULL), mailbox);
  MSG_comm_wait(res_irecv, -1);
  INFO0("I'm done. See you!");
  return 0;
}                               /* end_of_receiver */

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model","KCCFLN05"); */
  {                             /*  Simulation setting */
    MSG_set_channel_number(0);
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("sender", sender);
    MSG_function_register("receiver", receiver);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  SIMIX_message_sizes_output("toto.txt");
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
