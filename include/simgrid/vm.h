/* Public interface to the Virtual Machine datatype                         */

/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_VM_H_
#define INCLUDE_SIMGRID_VM_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL

/** @brief Create a virtual machine with a single core, on the given physical host */
XBT_PUBLIC sg_vm_t sg_vm_create_core(sg_host_t pm, const char* name);
/** @brief Create a virtual machine with the given amount of cores, on the given physical host */
XBT_PUBLIC sg_vm_t sg_vm_create_multicore(sg_host_t pm, const char* name, int core_amount);

/** @brief Returns whether this virtual machine was created but not started yet */
XBT_PUBLIC int sg_vm_is_created(const_sg_vm_t vm);
/** @brief Returns whether this virtual machine is currently running */
XBT_PUBLIC int sg_vm_is_running(const_sg_vm_t vm);
/** @brief Returns whether this virtual machine is currently suspended */
XBT_PUBLIC int sg_vm_is_suspended(const_sg_vm_t vm);

/** @brief Retrieves the name of this virtual machine */
XBT_PUBLIC const char* sg_vm_get_name(const_sg_vm_t vm);
/** @brief Sets the amount of RAM dedicated to this virtual machine */
XBT_PUBLIC void sg_vm_set_ramsize(sg_vm_t vm, size_t size);
/** @brief Retrieve the amount of RAM dedicated to this virtual machine */
XBT_PUBLIC size_t sg_vm_get_ramsize(const_sg_vm_t vm);
/** @brief Sets the CPU utilization bound for this virtual machine */
XBT_PUBLIC void sg_vm_set_bound(sg_vm_t vm, double bound);
/** @brief Retrieve the physical machine (the host) on which this virtual machine currently runs */
XBT_PUBLIC sg_host_t sg_vm_get_pm(const_sg_vm_t vm);

/** @brief Immediately boots this virtual machine, which must be in the "created" state */
XBT_PUBLIC void sg_vm_start(sg_vm_t vm);
/** @brief Suspends this virtual machine. Actors running on it are not scheduled anymore until resumed. */
XBT_PUBLIC void sg_vm_suspend(sg_vm_t vm);
/** @brief Resumes this virtual machine, previously suspended */
XBT_PUBLIC void sg_vm_resume(sg_vm_t vm);
/** @brief Immediately shuts down this virtual machine. Actors running on it are forcefully stopped. */
XBT_PUBLIC void sg_vm_shutdown(sg_vm_t vm);
/** @brief Immediately destroys this virtual machine, freeing its resources */
XBT_PUBLIC void sg_vm_destroy(sg_vm_t vm);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_VM_H_ */
