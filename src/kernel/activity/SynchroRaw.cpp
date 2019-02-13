/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroRaw.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_synchro);
namespace simgrid {
namespace kernel {
namespace activity {

RawImpl::~RawImpl()
{
  surf_action_->unref();
}
void RawImpl::suspend()
{
  /* The suspension of raw synchros is delayed to when the process is rescheduled. */
}

void RawImpl::resume()
{
  /* I cannot resume raw synchros directly. This is delayed to when the process is rescheduled at
   * the end of the synchro. */
}
void RawImpl::post()
{
  XBT_IN("(%p)",this);
  if (surf_action_->get_state() == resource::Action::State::FAILED)
    state_ = SIMIX_FAILED;
  else if (surf_action_->get_state() == resource::Action::State::FINISHED)
    state_ = SIMIX_SRC_TIMEOUT;

  SIMIX_synchro_finish(this);
  XBT_OUT();
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
