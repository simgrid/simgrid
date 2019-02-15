/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/CommImpl.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_network);

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_comm_copy_data_callback)(smx_activity_t, void*, size_t) = &SIMIX_comm_copy_pointer_callback;

void SIMIX_comm_set_copy_data_callback(void (*callback)(smx_activity_t, void*, size_t))
{
  SIMIX_comm_copy_data_callback = callback;
}

void SIMIX_comm_copy_pointer_callback(smx_activity_t synchro, void* buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  xbt_assert((buff_size == sizeof(void*)), "Cannot copy %zu bytes: must be sizeof(void*)", buff_size);
  *(void**)(comm->dst_buff_) = buff;
}

namespace simgrid {
namespace kernel {
namespace activity {

CommImpl::CommImpl(CommImpl::Type type) : type(type)
{
  state_   = SIMIX_WAITING;
  src_data_ = nullptr;
  dst_data_ = nullptr;
  XBT_DEBUG("Create comm activity %p", this);
}

CommImpl::~CommImpl()
{
  XBT_DEBUG("Really free communication %p", this);

  cleanupSurf();

  if (detached && state_ != SIMIX_DONE) {
    /* the communication has failed and was detached:
     * we have to free the buffer */
    if (clean_fun)
      clean_fun(src_buff_);
    src_buff_ = nullptr;
  }

  if (mbox)
    mbox->remove(this);
}

/**  @brief Starts the simulation of a communication synchro. */
void CommImpl::start()
{
  /* If both the sender and the receiver are already there, start the communication */
  if (state_ == SIMIX_READY) {

    s4u::Host* sender   = src_actor_->host_;
    s4u::Host* receiver = dst_actor_->host_;

    surf_action_ = surf_network_model->communicate(sender, receiver, task_size_, rate_);
    surf_action_->set_data(this);
    state_ = SIMIX_RUNNING;

    XBT_DEBUG("Starting communication %p from '%s' to '%s' (surf_action: %p)", this, sender->get_cname(),
              receiver->get_cname(), surf_action_);

    /* If a link is failed, detect it immediately */
    if (surf_action_->get_state() == resource::Action::State::FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure", sender->get_cname(),
                receiver->get_cname());
      state_ = SIMIX_LINK_FAILURE;
      cleanupSurf();
    }

    /* If any of the process is suspended, create the synchro but stop its execution,
       it will be restarted when the sender process resume */
    if (src_actor_->is_suspended() || dst_actor_->is_suspended()) {
      if (src_actor_->is_suspended())
        XBT_DEBUG("The communication is suspended on startup because src (%s@%s) was suspended since it initiated the "
                  "communication",
                  src_actor_->get_cname(), src_actor_->host_->get_cname());
      else
        XBT_DEBUG("The communication is suspended on startup because dst (%s@%s) was suspended since it initiated the "
                  "communication",
                  dst_actor_->get_cname(), dst_actor_->host_->get_cname());

      surf_action_->suspend();
    }
  }
}

/** @brief Copy the communication data from the sender's buffer to the receiver's one  */
void CommImpl::copy_data()
{
  size_t buff_size = src_buff_size_;
  /* If there is no data to copy then return */
  if (not src_buff_ || not dst_buff_ || copied)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)", this,
            src_actor_ ? src_actor_->host_->get_cname() : "a finished process", src_buff_,
            dst_actor_ ? dst_actor_->host_->get_cname() : "a finished process", dst_buff_, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (dst_buff_size_)
    buff_size = std::min(buff_size, *(dst_buff_size_));

  /* Update the receiver's buffer size to the copied amount */
  if (dst_buff_size_)
    *dst_buff_size_ = buff_size;

  if (buff_size > 0) {
    if (copy_data_fun)
      copy_data_fun(this, src_buff_, buff_size);
    else
      SIMIX_comm_copy_data_callback(this, src_buff_, buff_size);
  }

  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  copied = true;
}

void CommImpl::suspend()
{
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surf_action_)
    surf_action_->suspend();
  /* if not created yet, the action will be suspended on creation, in CommImpl::start() */
}

void CommImpl::resume()
{
  /*FIXME: check what happen with the timeouts */
  if (surf_action_)
    surf_action_->resume();
  /* in the other case, the synchro were not really suspended yet, see CommImpl::suspend() and CommImpl::start() */
}

void CommImpl::cancel()
{
  /* if the synchro is a waiting state means that it is still in a mbox so remove from it and delete it */
  if (state_ == SIMIX_WAITING) {
    mbox->remove(this);
    state_ = SIMIX_CANCELED;
  } else if (not MC_is_active() /* when running the MC there are no surf actions */
             && not MC_record_replay_is_active() && (state_ == SIMIX_READY || state_ == SIMIX_RUNNING)) {
    surf_action_->cancel();
  }
}

/**  @brief get the amount remaining from the communication */
double CommImpl::remains()
{
  return surf_action_->get_remains();
}

/** @brief This is part of the cleanup process, probably an internal command */
void CommImpl::cleanupSurf()
{
  if (surf_action_) {
    surf_action_->unref();
    surf_action_ = nullptr;
  }

  if (src_timeout_) {
    src_timeout_->unref();
    src_timeout_ = nullptr;
  }

  if (dst_timeout_) {
    dst_timeout_->unref();
    dst_timeout_ = nullptr;
  }
}

void CommImpl::post()
{
  /* Update synchro state */
  if (src_timeout_ && src_timeout_->get_state() == resource::Action::State::FINISHED)
    state_ = SIMIX_SRC_TIMEOUT;
  else if (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FINISHED)
    state_ = SIMIX_DST_TIMEOUT;
  else if (src_timeout_ && src_timeout_->get_state() == resource::Action::State::FAILED)
    state_ = SIMIX_SRC_HOST_FAILURE;
  else if (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FAILED)
    state_ = SIMIX_DST_HOST_FAILURE;
  else if (surf_action_ && surf_action_->get_state() == resource::Action::State::FAILED) {
    state_ = SIMIX_LINK_FAILURE;
  } else
    state_ = SIMIX_DONE;

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d", this, (int)state_,
            src_actor_.get(), dst_actor_.get(), detached);

  /* destroy the surf actions associated with the Simix communication */
  cleanupSurf();

  /* if there are simcalls associated with the synchro, then answer them */
  if (not simcalls_.empty()) {
    SIMIX_comm_finish(this);
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
