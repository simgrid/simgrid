/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/plugins/energy.h"
#include "simgrid/vm.h"
#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_vm, "Messages of this example");

static void worker_func(int argc, char* argv[])
{
  sg_actor_execute(300E6);
  XBT_INFO("This worker is done.");
}

static void dvfs(int argc, char* argv[])
{
  sg_host_t host1 = sg_host_by_name("MyHost1");
  sg_host_t host2 = sg_host_by_name("MyHost2");
  sg_host_t host3 = sg_host_by_name("MyHost3");

  /* Host 1 */
  XBT_INFO("Creating and starting two VMs");
  sg_vm_t vm_host1 = sg_vm_create_core(host1, "vm_host1");
  sg_vm_start(vm_host1);
  sg_vm_t vm_host2 = sg_vm_create_core(host2, "vm_host2");
  sg_vm_start(vm_host2);

  XBT_INFO("Create two tasks on Host1: both inside a VM");
  sg_actor_create("p11", (sg_host_t)vm_host1, worker_func, 0, NULL);
  sg_actor_create("p12", (sg_host_t)vm_host1, worker_func, 0, NULL);

  XBT_INFO("Create two tasks on Host2: one inside a VM, the other directly on the host");
  sg_actor_create("p21", (sg_host_t)vm_host2, worker_func, 0, NULL);
  sg_actor_create("p22", host2, worker_func, 0, NULL);

  XBT_INFO("Create two tasks on Host3: both directly on the host");
  sg_actor_create("p31", host3, worker_func, 0, NULL);
  sg_actor_create("p32", host3, worker_func, 0, NULL);

  XBT_INFO("Wait 5 seconds. The tasks are still running (they run for 3 seconds, but 2 tasks are co-located, "
           "so they run for 6 seconds)");
  sg_actor_sleep_for(5);
  XBT_INFO("Wait another 5 seconds. The tasks stop at some point in between");
  sg_actor_sleep_for(5);

  sg_vm_destroy(vm_host1);
  sg_vm_destroy(vm_host2);
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  simgrid_init(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("dvfs", sg_host_by_name("MyHost1"), dvfs, 0, NULL);

  simgrid_run();

  XBT_INFO("Total simulation time: %.2f; Host2 and Host3 must have the exact same energy consumption; Host1 is "
           "multi-core and will differ.",
           simgrid_get_clock());

  return 0;
}
