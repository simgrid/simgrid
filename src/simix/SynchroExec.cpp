/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroExec.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_host_private.h"

simgrid::simix::Exec::~Exec()
{
  if (surf_exec)
    surf_exec->unref();
}
void simgrid::simix::Exec::suspend()
{
  if (surf_exec)
    surf_exec->suspend();
}

void simgrid::simix::Exec::resume()
{
  if (surf_exec)
    surf_exec->resume();
}

double simgrid::simix::Exec::remains()
{
  if (state == SIMIX_RUNNING)
    return surf_exec->getRemains();

  return 0;
}

void simgrid::simix::Exec::post()
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
    surf_exec = NULL;
  }

  /* If there are simcalls associated with the synchro, then answer them */
  if (xbt_fifo_size(simcalls))
    SIMIX_execution_finish(this);
}
