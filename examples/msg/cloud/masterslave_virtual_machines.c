/* Copyright (c) 2007-2012. The SimGrid Team. All rights reserved.                             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  - <b>cloud/masterslave_virtual_machines.c: Master/slaves
 *    example, à la cloud</b>. The classical example revisited to demonstrate the use of virtual machines.
 */

double task_comp_size = 10000000;
double task_comm_size = 10000000;


int master(int argc, char *argv[]);
int slave_fun(int argc, char *argv[]);

static void work_batch(int slaves_count) {
  int i;
  for (i = 0; i < slaves_count; i++) {
    char taskname_buffer[64];
    char mailbox_buffer[64];

    sprintf(taskname_buffer, "Task_%d", i);
    sprintf(mailbox_buffer,"Slave_%d",i);

    XBT_INFO("Sending \"%s\" to \"%s\"",taskname_buffer,mailbox_buffer);
    MSG_task_send(MSG_task_create(taskname_buffer, task_comp_size, task_comm_size,NULL),
        mailbox_buffer);
  }
}

int master(int argc, char *argv[]) {
  int slaves_count = 10;
  msg_host_t *slaves = xbt_new(msg_host_t,10);

  msg_vm_t vm;
  unsigned int i;

  /* Retrive the hostnames constituting our playground today */
  for (i = 1; i < argc; i++) {
    slaves[i - 1] = MSG_get_host_by_name(argv[i]);
    xbt_assert(slaves[i - 1] != NULL, "Cannot use inexistent host %s", argv[i]);
  }

  /* Launch the sub processes: one VM per host, with one process inside each */

  for (i=0;i<slaves_count;i++) {
    char slavename[64];
    sprintf(slavename,"Slave %d",i);
    char**argv=xbt_new(char*,3);
    argv[0] = xbt_strdup(slavename);
    argv[1] = bprintf("%d",i);
    argv[2] = NULL;

    char vmName[64];
    snprintf(vmName, 64, "vm_%d", i);

    msg_vm_t vm = MSG_vm_start(slaves[i],vmName,2);
    MSG_vm_bind(vm, MSG_process_create_with_arguments(slavename,slave_fun,NULL,slaves[i],2,argv));
  }


  xbt_dynar_t vms = MSG_vms_as_dynar();
  XBT_INFO("Launched %ld VMs", xbt_dynar_length(vms));

  /* Send a bunch of work to every one */
  XBT_INFO("Send a first batch of work to every one");
  work_batch(slaves_count);

  XBT_INFO("Now suspend all VMs, just for fun");

  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_suspend(vm);
  }

  XBT_INFO("Wait a while");
  MSG_process_sleep(2);

  XBT_INFO("Enough. Let's resume everybody.");
  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_resume(vm);
  }
  XBT_INFO("Sleep long enough for everyone to be done with previous batch of work");
  MSG_process_sleep(1000-MSG_get_clock());

  XBT_INFO("Add one more process per VM");
  xbt_dynar_foreach(vms,i,vm) {
    msg_vm_t vm = xbt_dynar_get_as(vms,i,msg_vm_t);
    char slavename[64];
    sprintf(slavename,"Slave %ld",i+xbt_dynar_length(vms));
    char**argv=xbt_new(char*,3);
    argv[0] = xbt_strdup(slavename);
    argv[1] = bprintf("%ld",i+xbt_dynar_length(vms));
    argv[2] = NULL;
    MSG_vm_bind(vm, MSG_process_create_with_arguments(slavename,slave_fun,NULL,slaves[i],2,argv));
  }

  XBT_INFO("Reboot all the VMs");
  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_reboot(vm);
  }

  work_batch(slaves_count*2);

  XBT_INFO("Migrate everyone to the second host.");
  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_migrate(vm,slaves[1]);
  }
  XBT_INFO("Suspend everyone, move them to the third host, and resume them.");
  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_suspend(vm);
    MSG_vm_migrate(vm,slaves[2]);
    MSG_vm_resume(vm);
  }


  XBT_INFO("Let's shut down the simulation. 10 first processes will be shut down cleanly while the second half will forcefully get killed");
  for (i = 0; i < slaves_count; i++) {
    char mailbox_buffer[64];
    sprintf(mailbox_buffer,"Slave_%d",i);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox_buffer);
  }

  xbt_dynar_foreach(vms,i,vm) {
    MSG_vm_shutdown(vm);
    MSG_vm_destroy(vm);
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  xbt_dynar_free(&vms);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave_fun(int argc, char *argv[])
{
  char *mailbox_name;
  msg_task_t task = NULL;
  _XBT_GNUC_UNUSED int res;
  /* since the slaves will move around, use slave_%d as mailbox names instead of hostnames */
  xbt_assert(argc>=2, "slave processes need to be given their rank as parameter");
  mailbox_name=bprintf("Slave_%s",argv[1]);
  XBT_INFO("Slave listenning on %s",argv[1]);
  while (1) {
    res = MSG_task_receive(&(task),mailbox_name);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    XBT_INFO("Received \"%s\" from mailbox %s", MSG_task_get_name(task),mailbox_name);
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }

  free(mailbox_name);
  return 0;
}                               /* end_of_slave */

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  xbt_dynar_t hosts_dynar;
  msg_host_t*hosts= xbt_new(msg_host_t,10);
  char**hostnames= xbt_new(char*,10);
  char**masterargv=xbt_new(char*,12);
  int i;

  /* Get the arguments */
  MSG_init(&argc, argv);
  if (argc < 2) {
    printf("Usage: %s platform_file\n", argv[0]);
    printf("example: %s msg_platform.xml\n", argv[0]);
    exit(1);
  } if (argc>2) {
    printf("Usage: %s platform_file\n", argv[0]);
    printf("Other parameters (such as the deployment file) are ignored.");
  }

  /* load the platform file */
  MSG_create_environment(argv[1]);
  /* Retrieve the 10 first hosts of the platform file */
  hosts_dynar = MSG_hosts_as_dynar();
  xbt_assert(xbt_dynar_length(hosts_dynar)>10,
      "I need at least 10 hosts in the platform file, but %s contains only %ld hosts_dynar.",
      argv[1],xbt_dynar_length(hosts_dynar));
  for (i=0;i<10;i++) {
    hosts[i] = xbt_dynar_get_as(hosts_dynar,i,msg_host_t);
    hostnames[i] = xbt_strdup(MSG_host_get_name(hosts[i]));
  }
  masterargv[0]=xbt_strdup("master");
  for (i=1;i<11;i++) {
    masterargv[i] = xbt_strdup(MSG_host_get_name(hosts[i-1]));
  }
  masterargv[11]=NULL;
  MSG_process_create_with_arguments("master",master,NULL,hosts[0],11,masterargv);
  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());

  free(hosts);
  free(hostnames);
  xbt_dynar_free(&hosts_dynar);

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
