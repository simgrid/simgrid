/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int master(int argc, char *argv[])
{
  double task_comp_size = 5E7;
  double task_comm_size = 1E6;
  double timeout = 1;

  char mailbox[256];
  xbt_ex_t ex;

  sprintf(mailbox, "jupi");

  msg_task_t task = MSG_task_create("normal", task_comp_size, task_comm_size, NULL);
  XBT_INFO("Sending task: \"%s\"", task->name);
  MSG_task_send_with_timeout(task, mailbox, timeout);

  task = MSG_task_create("cancel directly", task_comp_size, task_comm_size, NULL);
  XBT_INFO("Canceling task \"%s\" directly", task->name);
  MSG_task_cancel(task);
  MSG_task_destroy(task);

  task = MSG_task_create("destroy directly", task_comp_size, task_comm_size, NULL);
  XBT_INFO("Destroying task \"%s\" directly", task->name);
  MSG_task_destroy(task);

  task = MSG_task_create("cancel", task_comp_size, task_comm_size, NULL);
  msg_comm_t comm = MSG_task_isend(task, mailbox);
  XBT_INFO("Canceling task \"%s\" during comm", task->name);
  MSG_task_cancel(task);
  TRY {
    MSG_comm_wait(comm, -1);
  }
  CATCH (ex) {
    xbt_ex_free(ex);
    MSG_comm_destroy(comm);
  }
  MSG_task_destroy(task);

  task = MSG_task_create("finalize", task_comp_size, task_comm_size, NULL);
  comm = MSG_task_isend(task, mailbox);
  XBT_INFO("Destroying task \"%s\" during comm", task->name);
  MSG_task_destroy(task);
  TRY {
    MSG_comm_wait(comm, -1);
  }
  CATCH (ex) {
    xbt_ex_free(ex);
    MSG_comm_destroy(comm);
  }

  task = MSG_task_create("cancel", task_comp_size, task_comm_size, NULL);
  MSG_task_send_with_timeout(task, mailbox, timeout);

  task = MSG_task_create("finalize", task_comp_size, task_comm_size, NULL);
  MSG_task_send_with_timeout(task, mailbox, timeout);

  XBT_INFO("Goodbye now!");
  return 0;
}

static int worker_main(int argc, char *argv[])
{
  msg_task_t task = MSG_process_get_data(MSG_process_self());
  msg_error_t res;
  XBT_INFO("Start %s", task->name);
  res = MSG_task_execute(task);
  XBT_INFO("Task %s", res == MSG_OK ? "done" : "failed");
  MSG_task_destroy(task);
  return 0;
}

static int slave(int argc, char *argv[])
{
  msg_task_t task;
  XBT_ATTRIB_UNUSED int res;
  int id = -1;
  char mailbox[80];
  double start, end;
  sprintf(mailbox, "jupi");

  while (1) {
    task = NULL;
    res = MSG_task_receive(&(task), mailbox);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Handling task \"%s\"", MSG_task_get_name(task));

    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      XBT_INFO("Destroying task \"%s\"", task->name);
      MSG_task_destroy(task);
      break;
    }

    if (!strcmp(MSG_task_get_name(task), "cancel")) {
      MSG_process_create("worker1", worker_main, task, MSG_host_self());
      MSG_process_sleep(0.1);
      XBT_INFO("Canceling task \"%s\"", task->name);
      MSG_task_cancel(task);
      continue;
    }

    start = MSG_get_clock();
    MSG_task_execute(task);
    end = MSG_get_clock();
    XBT_INFO("Task \"%s\" done in %f (amount %f)", MSG_task_get_name(task), end - start,
             MSG_task_get_flops_amount(task));

    MSG_task_destroy(task);
    task = NULL;
    id--;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("master", master, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("slave", slave, NULL, MSG_get_host_by_name("Jupiter"));

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
