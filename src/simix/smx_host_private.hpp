/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_HOST_PRIVATE_HPP
#define SIMIX_HOST_PRIVATE_HPP

#include <boost/intrusive/list.hpp>
#include <functional>
#include <map>
#include <vector>

#include <xbt/Extendable.hpp>
#include <xbt/base.h>

#include "popping_private.hpp"
#include "simgrid/simix.h"

#include "ActorImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"

/** @brief Host datatype from SIMIX POV */
namespace simgrid {
namespace simix {

class Host {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, Host> EXTENSION_ID;

  explicit Host();
  virtual ~Host();

  boost::intrusive::list<ActorImpl, boost::intrusive::member_hook<ActorImpl, boost::intrusive::list_member_hook<>,
                                                                  &ActorImpl::host_process_list_hook>>
      process_list;
  std::vector<ProcessArg*> auto_restart_processes;
  std::vector<ProcessArg*> boot_processes;

  void turnOn();
};
}
}

extern "C" {
XBT_PRIVATE void SIMIX_host_add_auto_restart_process(sg_host_t host, const char* name, std::function<void()> code,
                                                     void* data, double kill_time,
                                                     std::map<std::string, std::string>* properties, int auto_restart);

XBT_PRIVATE void SIMIX_host_autorestart(sg_host_t host);

XBT_PRIVATE void SIMIX_execution_finish(smx_activity_t synchro);

XBT_PRIVATE void SIMIX_set_category(smx_activity_t synchro, const char* category);
}

XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_start(const char* name, double flops_amount, double priority, double bound, sg_host_t host);
XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                               double* bytes_amount, double rate, double timeout);

#endif
