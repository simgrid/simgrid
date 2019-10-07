/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LIVE_MIGRATION_H_
#define SIMGRID_PLUGINS_LIVE_MIGRATION_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC void sg_vm_live_migration_plugin_init();
XBT_PRIVATE void sg_vm_dirty_page_tracking_init();
XBT_PUBLIC void sg_vm_start_dirty_page_tracking(sg_vm_t vm);
XBT_PUBLIC void sg_vm_stop_dirty_page_tracking(sg_vm_t vm);
XBT_PUBLIC double sg_vm_lookup_computed_flops(sg_vm_t vm);
XBT_PUBLIC void sg_vm_migrate(sg_vm_t vm, sg_host_t dst_pm);
XBT_PUBLIC void sg_vm_set_dirty_page_intensity(sg_vm_t vm, double intensity);
XBT_PUBLIC double sg_vm_get_dirty_page_intensity(sg_vm_t vm);
XBT_PUBLIC void sg_vm_set_working_set_memory(sg_vm_t vm, sg_size_t size);
XBT_PUBLIC sg_size_t sg_vm_get_working_set_memory(sg_vm_t vm);
XBT_PUBLIC void sg_vm_set_migration_speed(sg_vm_t vm, double speed);
XBT_PUBLIC double sg_vm_get_migration_speed(sg_vm_t vm);
XBT_PUBLIC double sg_vm_get_max_downtime(sg_vm_t vm);
XBT_PUBLIC int sg_vm_is_migrating(sg_vm_t vm);
XBT_PUBLIC sg_vm_t sg_vm_create_migratable(sg_host_t pm, const char* name, int coreAmount, int ramsize,
                                           int mig_netspeed, int dp_intensity);

#define MSG_vm_live_migration_plugin_init() sg_vm_live_migration_plugin_init()

#define MSG_vm_create_migratable(pm, name, coreAmount, ramsize, mig_netspeed, dp_intensity)                            \
  sg_vm_create_migratable((pm), (name), (coreAmount), (ramsize), (mig_netspeed), (dp_intensity))

#define MSG_vm_is_migrating(vm) sg_vm_is_migrating(vm)
#define MSG_vm_migrate(vm, dst_pm) sg_vm_migrate((vm), (dst_pm))

SG_END_DECL()

#endif
