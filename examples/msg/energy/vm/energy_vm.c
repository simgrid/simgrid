/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include<stdio.h>

#include "simgrid/msg.h"
#include "xbt/sysdep.h"         /* calloc */
#include "simgrid/plugins.h"

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

static msg_host_t host1 = NULL;
static msg_host_t host2 = NULL;
static msg_vm_t vm1 = NULL;

int dvfs(int argc, char *argv[]);

static int worker_func() {
	//MSG_process_on_exit(shutdown_vm, NULL);

	msg_task_t task1 = MSG_task_create("t1", 300E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);
  XBT_INFO("Worker fun done");
	return 0;
}

int dvfs(int argc, char *argv[])
{
  host1 = MSG_host_by_name("MyHost1");
  host2 = MSG_host_by_name("MyHost2");

  /* Host 1 */
  XBT_INFO("Creating and starting a VM");
	vm1 = MSG_vm_create(host1,
                      "vm1", /* name */
                      4, /* number of cpus */
                      2048, /* ram size */
                      100, /* net cap */
                      NULL, /* disk path */
                      1024 * 20, /* disk size */
                      10, /* mig netspeed */
                      50 /* dp intensity */
                      );

  // on Host1 create two tasks, one inside a VM  the other one  directly on the host 
	MSG_vm_start(vm1);
  MSG_process_create("p11", worker_func, NULL, vm1);
  MSG_process_create("p12", worker_func, NULL, host1);
  //XBT_INFO("Task on Host started");


  // on Host2, create two tasks directly on the host: Energy of host 1 and host 2 should be the same.
  MSG_process_create("p21", worker_func, NULL, host2);
  MSG_process_create("p22", worker_func, NULL, host2);

  /* Wait and see */
	MSG_process_sleep(5);
	MSG_process_sleep(5);
	MSG_vm_shutdown(vm1);

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  sg_energy_plugin_init();
  MSG_init(&argc, argv);

  if (argc != 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n",
              argv[0]);
    XBT_CRITICAL
        ("example: %s msg_platform.xml msg_deployment.xml\n",
         argv[0]);
    exit(1);
  }

  MSG_create_environment(argv[1]);

  /*   Application deployment */
  MSG_function_register("dvfs_test", dvfs);

  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Total simulation time: %.2f", MSG_get_clock());

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}

