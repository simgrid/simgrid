/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_synchro);

simgrid::kernel::activity::RawImpl::~RawImpl()
{
  sleep->unref();
}
void simgrid::kernel::activity::RawImpl::suspend()
{
  /* The suspension of raw synchros is delayed to when the process is rescheduled. */
}

void simgrid::kernel::activity::RawImpl::resume()
{
  /* I cannot resume raw synchros directly. This is delayed to when the process is rescheduled at
   * the end of the synchro. */
}
void simgrid::kernel::activity::RawImpl::post()
{
  XBT_IN("(%p)",this);
  if (sleep->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_FAILED;
  else if(sleep->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_SRC_TIMEOUT;

  SIMIX_synchro_finish(this);
  XBT_OUT();
}
