/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/simix/smx_host_private.h"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

simgrid::kernel::activity::ExecImpl::ExecImpl(const char* name, sg_host_t host) : host_(host)
{
  if (name)
    this->name = name;
  this->state  = SIMIX_RUNNING;
  XBT_DEBUG("Create exec %p", this);
}

simgrid::kernel::activity::ExecImpl::~ExecImpl()
{
  if (surf_exec)
    surf_exec->unref();
  if (timeoutDetector)
    timeoutDetector->unref();
  XBT_DEBUG("Destroy exec %p", this);
}
void simgrid::kernel::activity::ExecImpl::suspend()
{
  XBT_VERB("This exec is suspended (remain: %f)", surf_exec->getRemains());
  if (surf_exec)
    surf_exec->suspend();
}

void simgrid::kernel::activity::ExecImpl::resume()
{
  XBT_VERB("This exec is resumed (remain: %f)", surf_exec->getRemains());
  if (surf_exec)
    surf_exec->resume();
}

double simgrid::kernel::activity::ExecImpl::remains()
{
  if (state == SIMIX_RUNNING)
    return surf_exec->getRemains();

  return 0;
}

void simgrid::kernel::activity::ExecImpl::post()
{
  if (host_ && host_->isOff()) { /* FIXME: handle resource failure for parallel tasks too */
                                 /* If the host running the synchro failed, notice it. This way, the asking
                                  * process can be killed if it runs on that host itself */
    state = SIMIX_FAILED;
  } else if (surf_exec->getState() == simgrid::surf::Action::State::failed) {
    /* If the host running the synchro didn't fail, then the synchro was canceled */
    state = SIMIX_CANCELED;
  } else if (timeoutDetector && timeoutDetector->getState() == simgrid::surf::Action::State::done) {
    state = SIMIX_TIMEOUT;
  } else {
    state = SIMIX_DONE;
  }

  if (surf_exec) {
    surf_exec->unref();
    surf_exec = nullptr;
  }
  if (timeoutDetector) {
    timeoutDetector->unref();
    timeoutDetector = nullptr;
  }

  /* If there are simcalls associated with the synchro, then answer them */
  if (not simcalls.empty())
    SIMIX_execution_finish(this);
}
