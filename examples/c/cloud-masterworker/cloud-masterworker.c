/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/vm.h"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_masterworker, "Messages specific for this example");

#define MAXMBOXLEN 64
#define FINALIZE 221297 /* a magic number to tell people to stop working */

const double comp_size = 10000000;
const double comm_size = 10000000;

static void send_tasks(int nb_workers)
{
  for (int i = 0; i < nb_workers; i++) {
    char mbox_name[MAXMBOXLEN];
    snprintf(mbox_name, MAXMBOXLEN, "MBOX:WRK%02d", i);
    double* payload   = (double*)malloc(sizeof(double));
    *payload          = comp_size;
    sg_mailbox_t mbox = sg_mailbox_by_name(mbox_name);

    XBT_INFO("Send to mailbox(%s)", mbox_name);
    sg_mailbox_put(mbox, payload, comm_size);
  }
}

static void worker_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char* pr_name = sg_actor_self_get_name();
  char mbox_name[MAXMBOXLEN];
  snprintf(mbox_name, MAXMBOXLEN, "MBOX:%s", pr_name);
  sg_mailbox_t mbox = sg_mailbox_by_name(mbox_name);
  double* payload   = NULL;

  XBT_INFO("%s is listening on mailbox(%s)", pr_name, mbox_name);

  for (;;) {
    payload = (double*)sg_mailbox_get(mbox);

    XBT_INFO("%s received from mailbox(%s)", pr_name, mbox_name);

    if (*payload == FINALIZE) {
      free(payload);
      break;
    }

    sg_actor_execute(*payload);
    XBT_INFO("%s executed", pr_name);
    free(payload);
  }
}

static void master_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  unsigned int i;

  sg_host_t* worker_pms = sg_actor_self_data();

  sg_vm_t* vms = (sg_vm_t*)malloc(2 * sizeof(sg_vm_t));

  /* Launch VMs and worker actors. One VM per PM, and one worker actor per VM. */
  XBT_INFO("# Launch 2 VMs");
  for (int i = 0; i < 2; i++) {
    char* vm_name = bprintf("VM%02d", i);
    char* pr_name = bprintf("WRK%02d", i);

    sg_host_t pm = worker_pms[i];

    XBT_INFO("create %s on PM(%s)", vm_name, sg_host_get_name(pm));
    sg_vm_t vm = sg_vm_create_core(pm, vm_name);

    sg_vm_set_ramsize(vm, 1L * 1024 * 1024 * 1024); // 1GiB

    sg_vm_start(vm);
    vms[i] = vm;

    XBT_INFO("put an actor (%s) on %s", pr_name, vm_name);
    sg_actor_create(pr_name, (sg_host_t)vm, worker_fun, 0, NULL);

    xbt_free(vm_name);
    xbt_free(pr_name);
  }

  /* Send a bunch of work to every one */
  XBT_INFO("# Send to 2 worker actors");
  send_tasks(2);

  XBT_INFO("# Suspend all VMs");
  for (int i = 0; i < 2; i++) {
    XBT_INFO("suspend %s", sg_vm_get_name(vms[i]));
    sg_vm_suspend(vms[i]);
  }

  XBT_INFO("# Wait a while");
  sg_actor_sleep_for(2);

  XBT_INFO("# Resume all VMs");
  for (int i = 0; i < 2; i++) {
    sg_vm_resume(vms[i]);
  }

  XBT_INFO("# Sleep long enough for everyone to be done with previous batch of work");
  sg_actor_sleep_for(10 - simgrid_get_clock());

  XBT_INFO("# Add one more actor on each VM");
  for (unsigned int i = 0; i < 2; i++) {
    char* vm_name = bprintf("VM%02u", i);
    char* pr_name = bprintf("WRK%02u", i + 2);

    XBT_INFO("put an actor (%s) on %s", pr_name, vm_name);
    sg_actor_create(pr_name, (sg_host_t)vms[i], worker_fun, 0, NULL);

    free(vm_name);
    free(pr_name);
  }

  XBT_INFO("# Send to 4 worker actors");
  send_tasks(4);

  sg_host_t worker_pm0 = worker_pms[0];
  sg_host_t worker_pm1 = worker_pms[1];

  XBT_INFO("# Migrate all VMs to PM(%s)", sg_host_get_name(worker_pm0));
  for (int i = 0; i < 2; i++) {
    sg_vm_migrate(vms[i], worker_pm0);
  }

  XBT_INFO("# Migrate all VMs to PM(%s)", sg_host_get_name(worker_pm1));
  for (int i = 0; i < 2; i++) {
    sg_vm_migrate(vms[i], worker_pm1);
  }

  XBT_INFO("# Shutdown the half of worker actors gracefully. The remaining half will be forcibly killed.");
  for (i = 0; i < 2; i++) {
    char mbox_name[MAXMBOXLEN];
    snprintf(mbox_name, MAXMBOXLEN, "MBOX:WRK%02u", i);
    sg_mailbox_t mbox = sg_mailbox_by_name(mbox_name);
    double* payload   = (double*)malloc(sizeof(double));
    *payload          = FINALIZE;
    sg_mailbox_put(mbox, payload, 0);
  }

  XBT_INFO("# Wait a while before effective shutdown.");
  sg_actor_sleep_for(2);

  XBT_INFO("# Shutdown and destroy all the VMs. The remaining worker actors will be forcibly killed.");
  for (int i = 0; i < 2; i++) {
    XBT_INFO("shutdown %s", sg_vm_get_name(vms[i]));
    sg_vm_shutdown(vms[i]);
    XBT_INFO("destroy %s", sg_vm_get_name(vms[i]));
    sg_vm_destroy(vms[i]);
  }

  XBT_INFO("# Goodbye now!");
  free(vms);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  sg_vm_live_migration_plugin_init();

  xbt_assert(argc > 1, "Usage: %s example/platforms/cluster_backbone.xml\n", argv[0]);

  /* Load the platform file */
  simgrid_load_platform(argv[1]);

  /* Retrieve hosts from the platform file */
  sg_host_t* pms = sg_host_list();

  /* we need a master node and worker nodes */
  xbt_assert(sg_host_count() > 2, "need at least 3 hosts");

  /* the first pm is the master, the others are workers */
  sg_host_t master_pm = pms[0];

  sg_host_t* worker_pms = (sg_host_t*)malloc(2 * sizeof(sg_host_t));
  for (int i = 0; i < 2; i++)
    worker_pms[i] = pms[i + 1];

  free(pms);

  sg_actor_t actor = sg_actor_init("master", master_pm);
  sg_actor_data_set(actor, worker_pms);
  sg_actor_start(actor, master_fun, 0, NULL);

  simgrid_run();
  XBT_INFO("Bye (simulation time %g)", simgrid_get_clock());

  free(worker_pms);

  return 0;
}
