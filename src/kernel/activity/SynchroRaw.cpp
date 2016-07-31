/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_synchro_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_synchro);

simgrid::simix::Raw::~Raw()
{
  sleep->unref();
}
void simgrid::simix::Raw::suspend()
{
  /* The suspension of raw synchros is delayed to when the process is rescheduled. */
}

void simgrid::simix::Raw::resume()
{
  /* I cannot resume raw synchros directly. This is delayed to when the process is rescheduled at
   * the end of the synchro. */
}
void simgrid::simix::Raw::post()
{
  XBT_IN("(%p)",this);
  if (sleep->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_FAILED;
  else if(sleep->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_SRC_TIMEOUT;

  SIMIX_synchro_finish(this);
  XBT_OUT();
}
