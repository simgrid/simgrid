/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_VM_H_
#define INCLUDE_SIMGRID_VM_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL

/** @brief Opaque type describing a Virtual Machine.
 *  @ingroup msg_VMs
 *
 * All this is highly experimental and the interface will probably change in the future.
 * Please don't depend on this yet (although testing is welcomed if you feel so).
 * Usual lack of guaranty of any kind applies here, and is even increased.
 *
 */
XBT_PUBLIC sg_vm_t sg_vm_create_core(sg_host_t pm, const char* name);
XBT_PUBLIC sg_vm_t sg_vm_create_multicore(sg_host_t pm, const char* name, int coreAmount);

XBT_PUBLIC int sg_vm_is_created(sg_vm_t vm);
XBT_PUBLIC int sg_vm_is_running(sg_vm_t vm);
XBT_PUBLIC int sg_vm_is_suspended(sg_vm_t vm);

XBT_PUBLIC const char* sg_vm_get_name(const_sg_vm_t vm);
XBT_PUBLIC void sg_vm_set_ramsize(sg_vm_t vm, size_t size);
XBT_PUBLIC size_t sg_vm_get_ramsize(const_sg_vm_t vm);
XBT_PUBLIC void sg_vm_set_bound(sg_vm_t vm, double bound);
XBT_PUBLIC sg_host_t sg_vm_get_pm(const_sg_vm_t vm);

XBT_PUBLIC void sg_vm_start(sg_vm_t vm);
XBT_PUBLIC void sg_vm_suspend(sg_vm_t vm);
XBT_PUBLIC void sg_vm_resume(sg_vm_t vm);
XBT_PUBLIC void sg_vm_shutdown(sg_vm_t vm);
XBT_PUBLIC void sg_vm_destroy(sg_vm_t vm);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_VM_H_ */
