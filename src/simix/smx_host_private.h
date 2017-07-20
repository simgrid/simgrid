/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_HOST_PRIVATE_H
#define SIMIX_HOST_PRIVATE_H

#include <vector>
#include <functional>

#include <xbt/base.h>
#include <xbt/Extendable.hpp>

#include "simgrid/simix.h"
#include "popping_private.h"

#include "src/kernel/activity/ExecImpl.hpp"

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

SG_BEGIN_DECL()
XBT_PRIVATE void SIMIX_host_add_auto_restart_process(sg_host_t host,
                                         const char *name,
                                         std::function<void()> code,
                                         void *data,
                                         double kill_time,
                                         xbt_dict_t properties,
                                         int auto_restart);

XBT_PRIVATE void SIMIX_host_autorestart(sg_host_t host);
XBT_PRIVATE void SIMIX_execution_cancel(smx_activity_t synchro);
XBT_PRIVATE void SIMIX_execution_set_priority(smx_activity_t synchro, double priority);
XBT_PRIVATE void SIMIX_execution_set_bound(smx_activity_t synchro, double bound);

XBT_PRIVATE void SIMIX_execution_finish(simgrid::kernel::activity::ExecImplPtr exec);

XBT_PRIVATE void SIMIX_set_category(smx_activity_t synchro, const char *category);

SG_END_DECL()

XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_start(smx_actor_t issuer, const char* name, double flops_amount, double priority, double bound);
XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                               double* bytes_amount, double rate, double timeout);

#endif

