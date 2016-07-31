/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/host.hpp>

#include "src/kernel/activity/SynchroExec.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_host_private.h"

simgrid::kernel::activity::Exec::Exec(const char*name, sg_host_t hostarg)
{
  if (name)
    this->name = name;
  this->state = SIMIX_RUNNING;
  this->host = hostarg;
}

simgrid::kernel::activity::Exec::~Exec()
{
  if (surf_exec)
    surf_exec->unref();
}
void simgrid::kernel::activity::Exec::suspend()
{
  if (surf_exec)
    surf_exec->suspend();
}

void simgrid::kernel::activity::Exec::resume()
{
  if (surf_exec)
    surf_exec->resume();
}

double simgrid::kernel::activity::Exec::remains()
{
  if (state == SIMIX_RUNNING)
    return surf_exec->getRemains();

  return 0;
}

void simgrid::kernel::activity::Exec::post()
{
  if (host && host->isOff()) {/* FIMXE: handle resource failure for parallel tasks too */
    /* If the host running the synchro failed, notice it. This way, the asking
     * process can be killed if it runs on that host itself */
    state = SIMIX_FAILED;
  } else if (surf_exec->getState() == simgrid::surf::Action::State::failed) {
    /* If the host running the synchro didn't fail, then the synchro was canceled */
    state = SIMIX_CANCELED;
  } else {
    state = SIMIX_DONE;
  }

  if (surf_exec) {
    surf_exec->unref();
    surf_exec = nullptr;
  }

  /* If there are simcalls associated with the synchro, then answer them */
  if (!simcalls.empty())
    SIMIX_execution_finish(this);
}
