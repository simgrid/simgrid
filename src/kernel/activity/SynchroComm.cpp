/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroComm.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_network_private.h"
#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_network);

simgrid::kernel::activity::Comm::Comm(e_smx_comm_type_t _type) : type(_type)
{
  state = SIMIX_WAITING;
  src_data=nullptr;
  dst_data=nullptr;

  XBT_DEBUG("Create communicate synchro %p", this);
}

simgrid::kernel::activity::Comm::~Comm()
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

  if(mbox)
    SIMIX_mbox_remove(mbox, this);

}
void simgrid::kernel::activity::Comm::suspend()
{
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surf_comm)
    surf_comm->suspend();
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */

}

void simgrid::kernel::activity::Comm::resume()
{
  /*FIXME: check what happen with the timeouts */
  if (surf_comm)
    surf_comm->resume();
  /* in the other case, the synchro were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}

void simgrid::kernel::activity::Comm::cancel()
{
  /* if the synchro is a waiting state means that it is still in a mbox */
  /* so remove from it and delete it */
  if (state == SIMIX_WAITING) {
    SIMIX_mbox_remove(mbox, this);
    state = SIMIX_CANCELED;
  }
  else if (!MC_is_active() /* when running the MC there are no surf actions */
           && !MC_record_replay_is_active()
           && (state == SIMIX_READY || state == SIMIX_RUNNING)) {

    surf_comm->cancel();
  }
}

/**  @brief get the amount remaining from the communication */
double simgrid::kernel::activity::Comm::remains()
{
  switch (state) {

  case SIMIX_RUNNING:
    return surf_comm->getRemains();
    break;

  case SIMIX_WAITING:
  case SIMIX_READY:
    return 0; /*FIXME: check what should be returned */
    break;

  default:
    return 0; /*FIXME: is this correct? */
    break;
  }
}

/** @brief This is part of the cleanup process, probably an internal command */
void simgrid::kernel::activity::Comm::cleanupSurf()
{
  if (surf_comm){
    surf_comm->unref();
    surf_comm = nullptr;
  }

  if (src_timeout){
    src_timeout->unref();
    src_timeout = nullptr;
  }

  if (dst_timeout){
    dst_timeout->unref();
    dst_timeout = nullptr;
  }
}

void simgrid::kernel::activity::Comm::post()
{
  /* Update synchro state */
  if (src_timeout &&  src_timeout->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_SRC_TIMEOUT;
  else if (dst_timeout && dst_timeout->getState() == simgrid::surf::Action::State::done)
    state = SIMIX_DST_TIMEOUT;
  else if (src_timeout && src_timeout->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_SRC_HOST_FAILURE;
  else if (dst_timeout && dst_timeout->getState() == simgrid::surf::Action::State::failed)
    state = SIMIX_DST_HOST_FAILURE;
  else if (surf_comm && surf_comm->getState() == simgrid::surf::Action::State::failed) {
    state = SIMIX_LINK_FAILURE;
  } else
    state = SIMIX_DONE;

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d",
            this, (int)state, src_proc, dst_proc, detached);

  /* destroy the surf actions associated with the Simix communication */
  cleanupSurf();

  /* if there are simcalls associated with the synchro, then answer them */
  if (!simcalls.empty())
    SIMIX_comm_finish(this);
}
