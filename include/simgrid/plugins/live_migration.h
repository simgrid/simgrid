/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LIVE_MIGRATION_H_
#define SIMGRID_PLUGINS_LIVE_MIGRATION_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_vm_live_migration_plugin_init();
XBT_PUBLIC(void) sg_vm_start_dirty_page_tracking(sg_vm_t vm);
XBT_PUBLIC(void) sg_vm_stop_dirty_page_tracking(sg_vm_t vm);
XBT_PUBLIC(double) sg_vm_lookup_computed_flops(sg_vm_t vm);
XBT_PUBLIC(void) sg_vm_migrate(sg_vm_t vm, sg_host_t dst_pm);

#define MSG_vm_live_migration_plugin_init() sg_vm_live_migration_plugin_init()
#define MSG_vm_migrate(vm, dst_pm) sg_vm_migrate(vm, dst_pm)

SG_END_DECL()

#endif
