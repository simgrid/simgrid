/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_HOST_PRIVATE_HPP
#define SIMIX_HOST_PRIVATE_HPP

#include "src/simix/ActorImpl.hpp"

#include <boost/intrusive/list.hpp>

XBT_PRIVATE void SIMIX_host_add_auto_restart_process(sg_host_t host, simgrid::kernel::actor::ActorImpl* actor);
XBT_PRIVATE void SIMIX_host_autorestart(sg_host_t host);

XBT_PRIVATE void SIMIX_execution_finish(smx_activity_t synchro);

XBT_PRIVATE void SIMIX_set_category(smx_activity_t synchro, std::string category);

XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_start(std::string name, double flops_amount, double priority, double bound, sg_host_t host);
XBT_PRIVATE boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_parallel_start(std::string name, int host_nb, sg_host_t* host_list, double* flops_amount,
                               double* bytes_amount, double rate, double timeout);

#endif
