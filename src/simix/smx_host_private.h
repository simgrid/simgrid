/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_HOST_PRIVATE_H
#define _SIMIX_HOST_PRIVATE_H

#include <vector>
#include <functional>

#include <xbt/base.h>
#include <xbt/Extendable.hpp>

#include "simgrid/simix.h"
#include "popping_private.h"

#include "src/kernel/activity/SynchroExec.hpp"

/** @brief Host datatype from SIMIX POV */
namespace simgrid {
  namespace simix {
    class ProcessArg;

    class Host {
    public:
      static simgrid::xbt::Extension<simgrid::s4u::Host, Host> EXTENSION_ID;

      explicit Host();
      virtual ~Host();

      xbt_swag_t process_list;
      std::vector<ProcessArg*> auto_restart_processes;
      std::vector<ProcessArg*> boot_processes;

      void turnOn();
    };
  }
}
typedef simgrid::simix::Host s_smx_host_priv_t;

SG_BEGIN_DECL()
XBT_PRIVATE void _SIMIX_host_free_process_arg(void *);

XBT_PRIVATE void SIMIX_host_add_auto_restart_process(sg_host_t host,
                                         const char *name,
                                         std::function<void()> code,
                                         void *data,
                                         double kill_time,
                                         xbt_dict_t properties,
                                         int auto_restart);

XBT_PRIVATE void SIMIX_host_autorestart(sg_host_t host);
XBT_PRIVATE smx_activity_t SIMIX_execution_start(smx_actor_t issuer, const char *name,
    double flops_amount, double priority, double bound);
XBT_PRIVATE smx_activity_t SIMIX_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list,
                                                          double* flops_amount, double* bytes_amount, double amount,
                                                          double rate, double timeout);
XBT_PRIVATE void SIMIX_execution_cancel(smx_activity_t synchro);
XBT_PRIVATE void SIMIX_execution_set_priority(smx_activity_t synchro, double priority);
XBT_PRIVATE void SIMIX_execution_set_bound(smx_activity_t synchro, double bound);

XBT_PRIVATE void SIMIX_execution_finish(simgrid::kernel::activity::Exec *exec);

XBT_PRIVATE void SIMIX_set_category(smx_activity_t synchro, const char *category);

/* vm related stuff */
XBT_PRIVATE void SIMIX_vm_destroy(sg_host_t ind_vm);
// --
XBT_PRIVATE void SIMIX_vm_resume(sg_host_t ind_vm, smx_actor_t issuer);

XBT_PRIVATE void SIMIX_vm_suspend(sg_host_t ind_vm, smx_actor_t issuer);
// --
XBT_PRIVATE void SIMIX_vm_save(sg_host_t ind_vm, smx_actor_t issuer);

XBT_PRIVATE void SIMIX_vm_restore(sg_host_t ind_vm, smx_actor_t issuer);
// --
XBT_PRIVATE void SIMIX_vm_start(sg_host_t ind_vm);

XBT_PRIVATE void SIMIX_vm_shutdown(sg_host_t ind_vm, smx_actor_t issuer);
// --

XBT_PRIVATE e_surf_vm_state_t SIMIX_vm_get_state(sg_host_t ind_vm);
// --
XBT_PRIVATE void SIMIX_vm_migrate(sg_host_t ind_vm, sg_host_t ind_dst_pm);

XBT_PRIVATE void *SIMIX_vm_get_pm(sg_host_t ind_vm);

XBT_PRIVATE void SIMIX_vm_set_bound(sg_host_t ind_vm, double bound);

XBT_PRIVATE void SIMIX_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm);

SG_END_DECL()

#endif

