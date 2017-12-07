/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/CommImpl.hpp"

#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_network_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_network);

simgrid::kernel::activity::CommImpl::CommImpl(e_smx_comm_type_t _type) : type(_type)
{
  state    = SIMIX_WAITING;
  src_data = nullptr;
  dst_data = nullptr;
  XBT_DEBUG("Create comm activity %p", this);
}

simgrid::kernel::activity::CommImpl::~CommImpl()
{
  XBT_DEBUG("Really free communication %p", this);

  cleanupSurf();

  if (detached && state != SIMIX_DONE) {
    /* the communication has failed and was detached:
     * we have to free the buffer */
    if (clean_fun)
      clean_fun(src_buff);
    src_buff = nullptr;
  }

  if (mbox)
    mbox->remove(this);
}

void simgrid::kernel::activity::CommImpl::suspend()
{
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surfAction_)
    surfAction_->suspend();
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */
}

void simgrid::kernel::activity::CommImpl::resume()
{
  /*FIXME: check what happen with the timeouts */
  if (surfAction_)
    surfAction_->resume();
  /* in the other case, the synchro were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}

void simgrid::kernel::activity::CommImpl::cancel()
{
  /* if the synchro is a waiting state means that it is still in a mbox */
  /* so remove from it and delete it */
  if (state == SIMIX_WAITING) {
    mbox->remove(this);
    state = SIMIX_CANCELED;
  } else if (not MC_is_active() /* when running the MC there are no surf actions */
             && not MC_record_replay_is_active() && (state == SIMIX_READY || state == SIMIX_RUNNING)) {

    surfAction_->cancel();
  }
}

/**  @brief get the amount remaining from the communication */
double simgrid::kernel::activity::CommImpl::remains()
{
  return surfAction_->getRemains();
}

/** @brief This is part of the cleanup process, probably an internal command */
void simgrid::kernel::activity::CommImpl::cleanupSurf()
{
  if (surfAction_) {
    surfAction_->unref();
    surfAction_ = nullptr;
  }

  if (src_timeout) {
    src_timeout->unref();
    src_timeout = nullptr;
  }

  if (dst_timeout) {
    dst_timeout->unref();
    dst_timeout = nullptr;
  }
}

void simgrid::kernel::activity::CommImpl::post()
{
  /* Update synchro state */
  if (src_timeout && src_timeout->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_SRC_TIMEOUT;
  else if (dst_timeout && dst_timeout->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_DST_TIMEOUT;
  else if (src_timeout && src_timeout->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_SRC_HOST_FAILURE;
  else if (dst_timeout && dst_timeout->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_DST_HOST_FAILURE;
  else if (surfAction_ && surfAction_->getState() == simgrid::surf::Action::State::failed) {
    state = SIMIX_LINK_FAILURE;
  } else
    state = SIMIX_DONE;

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d", this, (int)state, src_proc,
            dst_proc, detached);

  /* destroy the surf actions associated with the Simix communication */
  cleanupSurf();

  /* if there are simcalls associated with the synchro, then answer them */
  if (not simcalls.empty()) {
    SIMIX_comm_finish(this);
  }
}
