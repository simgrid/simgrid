/* Copyright (c) 2007-2012. The SimGrid Team. All rights reserved.                             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  - <b>cloud/masterslave_virtual_machines.c: Master/workers
 *    example on a cloud</b>. The classical example revisited to demonstrate the use of virtual machines.
 */

double task_comp_size = 10000000;
double task_comm_size = 10000000;


int master_fun(int argc, char *argv[]);
int worker_fun(int argc, char *argv[]);

int nb_hosts=3;
//int nb_vms=nb_host*2; // 2 VMs per PM
 
static void work_batch(int workers_count)
{
  int i;
  for (i = 0; i < workers_count; i++) {
    char *tname = bprintf("Task%02d", i);
    char *mbox =  bprintf("MBOX:WRK%02d", i);

    msg_task_t task = MSG_task_create(tname, task_comp_size, task_comm_size, NULL);

    XBT_INFO("send task(%s) to mailbox(%s)", tname, mbox);
    MSG_task_send(task, mbox);

    free(tname);
    free(mbox);
  }
}

int master_fun(int argc, char *argv[])
{
  msg_vm_t vm;
  unsigned int i;
  int workers_count = argc - 1;

  msg_host_t *pms = xbt_new(msg_host_t, workers_count);
  xbt_dynar_t vms = xbt_dynar_new(sizeof(msg_vm_t), NULL);

  /* Retrive the PMs that launch worker processes. */
  for (i = 1; i < argc; i++)
    pms[i - 1] = MSG_get_host_by_name(argv[i]);


  /* Launch VMs and worker processes. One VM per PM, and one worker process per VM. */

  XBT_INFO("Launch %ld VMs", workers_count);
  for (i=0; i< workers_count; i++) {
    char *vm_name = bprintf("VM%02d", i);
    char *pr_name = bprintf("WRK%02d", i);
    char *mbox = bprintf("MBOX:WRK%02d", i);

    char **wrk_argv = xbt_new(char*, 3);
    wrk_argv[0] = pr_name;
    wrk_argv[1] = mbox;
    wrk_argv[2] = NULL;

    XBT_INFO("create %s", vm_name);
    msg_vm_t vm = MSG_vm_create_core(pms[i], vm_name);
    MSG_vm_start(vm);
    xbt_dynar_push(vms, &vm);

    XBT_INFO("put %s on %s", pr_name, vm_name);
    MSG_process_create_with_arguments(pr_name, worker_fun, NULL, vm, 2, wrk_argv);
  }


  /* Send a bunch of work to every one */
  XBT_INFO("Send a task to %d worker process", workers_count);
  work_batch(workers_count);

  XBT_INFO("Suspend all VMs");
  xbt_dynar_foreach(vms, i, vm) {
    const char *vm_name = MSG_host_get_name(vm);
    XBT_INFO("suspend %s", vm_name);
    MSG_vm_suspend(vm);
  }

  XBT_INFO("Wait a while");
  MSG_process_sleep(2);

  XBT_INFO("Resume all VMs");
  xbt_dynar_foreach(vms, i, vm) {
    MSG_vm_resume(vm);
  }


  XBT_INFO("Sleep long enough for everyone to be done with previous batch of work");
  MSG_process_sleep(1000 - MSG_get_clock());

  XBT_INFO("Add one more process per VM");
  xbt_dynar_foreach(vms, i, vm) {
    unsigned int index = i + xbt_dynar_length(vms);
    char *vm_name = bprintf("VM%02d", i);
    char *pr_name = bprintf("WRK%02d", index);
    char *mbox = bprintf("MBOX:WRK%02d", index);

    char **wrk_argv = xbt_new(char*, 3);
    wrk_argv[0] = pr_name;
    wrk_argv[1] = mbox;
    wrk_argv[2] = NULL;

    XBT_INFO("put %s on %s", pr_name, vm_name);
    MSG_process_create_with_arguments(pr_name, worker_fun, NULL, vm, 2, wrk_argv);
  }

  XBT_INFO("Send a task to %d worker process", workers_count * 2);
  work_batch(workers_count * 2);

  XBT_INFO("Migrate all VMs to PM(%s)", MSG_host_get_name(pms[1]));
  xbt_dynar_foreach(vms, i, vm) {
    MSG_vm_migrate(vm, pms[1]);
  }

  /* Migration with default policy is called (i.e. live migration with pre-copy strategy */
  /* If you want to use other policy such as post-copy or cold migration, you should add a third parameter that defines the policy */
  XBT_INFO("Migrate all VMs to PM(%s)", MSG_host_get_name(pms[2]));
  xbt_dynar_foreach(vms, i, vm) {
    // MSG_vm_suspend(vm);
    MSG_vm_migrate(vm, pms[2]);
    // MSG_vm_resume(vm);
  }


  XBT_INFO("Shutdown the first worker processes gracefuly. The   the second half will forcefully get killed");
  for (i = 0; i < workers_count; i++) {
    char mbox[64];
    sprintf(mbox, "MBOX:WRK%02d", i);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mbox);
  }

  XBT_INFO("Wait a while before effective shutdown.");
  MSG_process_sleep(2);


  XBT_INFO("Shutdown and destroy all the VMs. The remaining worker processes will be forcibly killed.");
  xbt_dynar_foreach(vms, i, vm) {
    XBT_INFO("shutdown %s", MSG_host_get_name(vm));
    MSG_vm_shutdown(vm);
    XBT_INFO("destroy %s", MSG_host_get_name(vm));
    MSG_vm_destroy(vm);
  }

  XBT_INFO("Goodbye now!");
  free(pms);
  xbt_dynar_free(&vms);

  return 0;
}

/** Receiver function  */
int worker_fun(int argc, char *argv[])
{
  xbt_assert(argc == 2, "need mbox in arguments");

  char *mbox = argv[1];
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  XBT_INFO("%s is listenning on mailbox(%s)", pr_name, mbox);

  for (;;) {
    msg_task_t task = NULL;

    msg_error_t res = MSG_task_receive(&task, mbox);
    if (res != MSG_OK) {
      XBT_CRITICAL("MSG_task_get failed");
      DIE_IMPOSSIBLE;
    }

    XBT_INFO("%s received task(%s) from mailbox(%s)",
        pr_name, MSG_task_get_name(task), mbox);

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

/** Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  if (argc != 2) {
    printf("Usage: %s example/msg/msg_platform.xml\n", argv[0]);
    return 1;
  }
 /* Load the platform file */
  MSG_create_environment(argv[1]);

  /* Retrieve the  first hosts from the platform file */
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();

  if (xbt_dynar_length(hosts_dynar) <= nb_hosts) {
    XBT_CRITICAL("need %d hosts", nb_hosts);
    return 1;
  }

  msg_host_t master_pm;
  char **master_argv = xbt_new(char *, 12);
  master_argv[0] = xbt_strdup("master");
  master_argv[11] = NULL;

  unsigned int i;
  msg_host_t host;
  xbt_dynar_foreach(hosts_dynar, i, host) {
    if (i == 0) {
      master_pm = host;
      continue;
    }

    master_argv[i] = xbt_strdup(MSG_host_get_name(host));

    if (i == nb_hosts)
      break;
  }


  MSG_process_create_with_arguments("master", master_fun, NULL, master_pm, nb_hosts+1, master_argv);

  msg_error_t res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());

  xbt_dynar_free(&hosts_dynar);

  return !(res == MSG_OK);
}
