/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroRaw.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/kernel/context/Context.hpp"
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
  smx_simcall_t simcall = simcalls_.front();
  simcalls_.pop_front();

  SIMIX_synchro_stop_waiting(simcall->issuer, simcall);
  simcall->issuer->waiting_synchro = nullptr;

  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    state_ = SIMIX_FAILED;
    simcall->issuer->context_->iwannadie = true;
  } else if (surf_action_->get_state() == resource::Action::State::FINISHED) {
    state_ = SIMIX_SRC_TIMEOUT;
    SIMIX_simcall_answer(simcall);
  }
  XBT_OUT();
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
