/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define MAXMBOXLEN 64

/** @addtogroup MSG_examples
 *
 *  - <b>cloud/masterslave_virtual_machines.c: Master/workers
 *    example on a cloud</b>. The classical example revisited to demonstrate the use of virtual machines.
 */

const double task_comp_size = 10000000;
const double task_comm_size = 10000000;

static void send_tasks(int nb_workers)
{
  for (int i = 0; i < nb_workers; i++) {
    char *tname = bprintf("Task%02d", i);
    char *mbox  = bprintf("MBOX:WRK%02d", i);

    msg_task_t task = MSG_task_create(tname, task_comp_size, task_comm_size, NULL);

    XBT_INFO("Send task(%s) to mailbox(%s)", tname, mbox);
    MSG_task_send(task, mbox);

    free(tname);
    free(mbox);
  }
}

static int worker_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  char mbox[MAXMBOXLEN];
  snprintf(mbox, MAXMBOXLEN, "MBOX:%s", pr_name);

  XBT_INFO("%s is listening on mailbox(%s)", pr_name, mbox);

  for (;;) {
    msg_task_t task = NULL;

    msg_error_t res = MSG_task_receive(&task, mbox);
    if (res != MSG_OK) {
      XBT_CRITICAL("MSG_task_get failed");
      DIE_IMPOSSIBLE;
    }

    XBT_INFO("%s received task(%s) from mailbox(%s)", pr_name, MSG_task_get_name(task), mbox);

    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    MSG_task_execute(task);
    XBT_INFO("%s executed task(%s)", pr_name, MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  return 0;
}

static int master_fun(int argc, char *argv[])
{
  msg_vm_t vm;
  unsigned int i;

  xbt_dynar_t worker_pms = MSG_process_get_data(MSG_process_self());
  int nb_workers = xbt_dynar_length(worker_pms);

  xbt_dynar_t vms = xbt_dynar_new(sizeof(msg_vm_t), NULL);

  /* Launch VMs and worker processes. One VM per PM, and one worker process per VM. */
  XBT_INFO("# Launch %d VMs", nb_workers);
  for (int i = 0; i< nb_workers; i++) {
    char *vm_name = bprintf("VM%02d", i);
    char *pr_name = bprintf("WRK%02d", i);

    msg_host_t pm = xbt_dynar_get_as(worker_pms, i, msg_host_t);

    XBT_INFO("create %s on PM(%s)", vm_name, MSG_host_get_name(pm));
    msg_vm_t vm = MSG_vm_create_core(pm, vm_name);

    s_vm_params_t params;
    memset(&params, 0, sizeof(params));
    params.ramsize = 1L * 1024 * 1024 * 1024; // 1Gbytes
    MSG_host_set_params(vm, &params);

    MSG_vm_start(vm);
    xbt_dynar_push(vms, &vm);

    XBT_INFO("put a process (%s) on %s", pr_name, vm_name);
    MSG_process_create(pr_name, worker_fun, NULL, vm);

    xbt_free(vm_name);
    xbt_free(pr_name);
  }

  /* Send a bunch of work to every one */
  XBT_INFO("# Send a task to %d worker process", nb_workers);
  send_tasks(nb_workers);

  XBT_INFO("# Suspend all VMs");
  xbt_dynar_foreach(vms, i, vm) {
    const char *vm_name = MSG_host_get_name(vm);
    XBT_INFO("suspend %s", vm_name);
    MSG_vm_suspend(vm);
  }

  XBT_INFO("# Wait a while");
  MSG_process_sleep(2);

  XBT_INFO("# Resume all VMs");
  xbt_dynar_foreach(vms, i, vm) {
    MSG_vm_resume(vm);
  }

  XBT_INFO("# Sleep long enough for everyone to be done with previous batch of work");
  MSG_process_sleep(1000 - MSG_get_clock());

  XBT_INFO("# Add one more process on each VM");
  xbt_dynar_foreach(vms, i, vm) {
    unsigned int index = i + xbt_dynar_length(vms);
    char *vm_name = bprintf("VM%02d", i);
    char *pr_name = bprintf("WRK%02d", index);

    XBT_INFO("put a process (%s) on %s", pr_name, vm_name);
    MSG_process_create(pr_name, worker_fun, NULL, vm);

    xbt_free(vm_name);
    xbt_free(pr_name);
  }

  XBT_INFO("# Send a task to %d worker process", nb_workers * 2);
  send_tasks(nb_workers * 2);

  msg_host_t worker_pm0 = xbt_dynar_get_as(worker_pms, 0, msg_host_t);
  msg_host_t worker_pm1 = xbt_dynar_get_as(worker_pms, 1, msg_host_t);

  XBT_INFO("# Migrate all VMs to PM(%s)", MSG_host_get_name(worker_pm0));
  xbt_dynar_foreach(vms, i, vm) {
    MSG_vm_migrate(vm, worker_pm0);
  }

  XBT_INFO("# Migrate all VMs to PM(%s)", MSG_host_get_name(worker_pm1));
  xbt_dynar_foreach(vms, i, vm) {
    MSG_vm_migrate(vm, worker_pm1);
  }

  XBT_INFO("# Shutdown the half of worker processes gracefuly. The remaining half will be forcibly killed.");
  for (i = 0; i < nb_workers; i++) {
    char mbox[MAXMBOXLEN];
    snprintf(mbox, MAXMBOXLEN, "MBOX:WRK%02d", i);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mbox);
  }

  XBT_INFO("# Wait a while before effective shutdown.");
  MSG_process_sleep(2);

  XBT_INFO("# Shutdown and destroy all the VMs. The remaining worker processes will be forcibly killed.");
  xbt_dynar_foreach(vms, i, vm) {
    XBT_INFO("shutdown %s", MSG_host_get_name(vm));
    MSG_vm_shutdown(vm);
    XBT_INFO("destroy %s", MSG_host_get_name(vm));
    MSG_vm_destroy(vm);
  }

  XBT_INFO("# Goodbye now!");
  xbt_dynar_free(&vms);
  return 0;
}

/** Receiver function  */
int main(int argc, char *argv[])
{
  const int nb_workers = 2;

  MSG_init(&argc, argv);
  xbt_assert(argc >1,"Usage: %s example/msg/msg_platform.xml\n", argv[0]);

  /* Load the platform file */
  MSG_create_environment(argv[1]);

  /* Retrieve hosts from the platform file */
  xbt_dynar_t pms = MSG_hosts_as_dynar();

  /* we need a master node and worker nodes */
  xbt_assert(xbt_dynar_length(pms) > nb_workers,"need %d hosts", nb_workers + 1);

  /* the first pm is the master, the others are workers */
  msg_host_t master_pm = xbt_dynar_get_as(pms, 0, msg_host_t);

  xbt_dynar_t worker_pms = xbt_dynar_new(sizeof(msg_host_t), NULL);
  for (int i = 1; i < nb_workers + 1; i++) {
    msg_host_t pm = xbt_dynar_get_as(pms, i, msg_host_t);
    xbt_dynar_push(worker_pms, &pm);
  }

  /* Start the master process on the master pm. */
  MSG_process_create("master", master_fun, worker_pms, master_pm);

  msg_error_t res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  xbt_dynar_free(&worker_pms);
  xbt_dynar_free(&pms);

  return !(res == MSG_OK);
}
