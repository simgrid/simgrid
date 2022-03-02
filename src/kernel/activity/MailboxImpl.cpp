/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"

#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_mailbox, kernel, "Mailbox implementation");

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

namespace simgrid {
namespace kernel {
namespace activity {

unsigned MailboxImpl::next_id_ = 0;

MailboxImpl::~MailboxImpl()
{
  clear();
  set_receiver(nullptr);
}

/** @brief set the receiver of the mailbox to allow eager sends
 *  @param actor The receiving dude
 */
void MailboxImpl::set_receiver(s4u::ActorPtr actor)
{
  if (this->permanent_receiver_) {
    std::vector<MailboxImpl*>& mboxes = this->permanent_receiver_->mailboxes;
    mboxes.erase(std::remove(mboxes.begin(), mboxes.end(), this), mboxes.end());
  }

  if (actor != nullptr)
    this->permanent_receiver_ = actor->get_impl();
  else
    this->permanent_receiver_ = nullptr;
}
/** @brief Pushes a communication activity into a mailbox
 *  @param comm What to add
 */
void MailboxImpl::push(CommImplPtr comm)
{
  comm->set_mailbox(this);
  this->comm_queue_.push_back(std::move(comm));
}

/** @brief Removes a communication activity from a mailbox
 *  @param comm What to remove
 */
void MailboxImpl::remove(const CommImplPtr& comm)
{
  xbt_assert(comm->get_mailbox() == this, "Comm %p is in mailbox %s, not mailbox %s", comm.get(),
             (comm->get_mailbox() ? comm->get_mailbox()->get_cname() : "(null)"), this->get_cname());

  comm->set_mailbox(nullptr);
  for (auto it = this->comm_queue_.begin(); it != this->comm_queue_.end(); it++)
    if (*it == comm) {
      this->comm_queue_.erase(it);
      return;
    }
  xbt_die("Comm %p not found in mailbox %s", comm.get(), this->get_cname());
}

/** @brief Removes all communication activities from a mailbox
 */
void MailboxImpl::clear()
{
  for (auto comm : done_comm_queue_) {
    comm->cancel();
    comm->set_state(State::DST_HOST_FAILURE);
  }
  done_comm_queue_.clear();

  // CommImpl::cancel() will remove the comm from the mailbox..
  while (not comm_queue_.empty()) {
    auto comm = comm_queue_.back();
    if (comm->get_state() == State::WAITING && not comm->is_detached()) {
      comm->cancel();
      comm->set_state(State::DST_HOST_FAILURE);
    } else
      comm_queue_.pop_back();
  }
}

CommImplPtr MailboxImpl::iprobe(int type, bool (*match_fun)(void*, void*, CommImpl*), void* data)
{
  XBT_DEBUG("iprobe from %p %p", this, &comm_queue_);

  CommImplPtr this_comm(new CommImpl);
  CommImplType other_type;
  if (type == 1) {
    this_comm->set_type(CommImplType::SEND);
    other_type = CommImplType::RECEIVE;
  } else {
    this_comm->set_type(CommImplType::RECEIVE);
    other_type = CommImplType::SEND;
  }
  CommImplPtr other_comm = nullptr;
  if (permanent_receiver_ != nullptr && not done_comm_queue_.empty()) {
    XBT_DEBUG("first check in the permanent recv mailbox, to see if we already got something");
    other_comm = find_matching_comm(other_type, match_fun, data, this_comm, /*done*/ true, /*remove_matching*/ false);
  }
  if (not other_comm) {
    XBT_DEBUG("check if we have more luck in the normal mailbox");
    other_comm = find_matching_comm(other_type, match_fun, data, this_comm, /*done*/ false, /*remove_matching*/ false);
  }

  return other_comm;
}

/**
 *  @brief Checks if there is a communication activity queued in comm_queue_ matching our needs
 *  @param type The type of communication we are looking for (comm_send, comm_recv)
 *  @param match_fun the function to apply
 *  @param this_user_data additional parameter to the match_fun
 *  @param my_synchro what to compare against
 *  @param remove_matching whether or not to clean the found object from the queue
 *  @return The communication activity if found, nullptr otherwise
 */
CommImplPtr MailboxImpl::find_matching_comm(CommImplType type, bool (*match_fun)(void*, void*, CommImpl*),
                                            void* this_user_data, const CommImplPtr& my_synchro, bool done,
                                            bool remove_matching)
{
  auto& comm_queue      = done ? done_comm_queue_ : comm_queue_;

  auto iter = std::find_if(
      comm_queue.begin(), comm_queue.end(), [&type, &match_fun, &this_user_data, &my_synchro](const CommImplPtr& comm) {
        void* other_user_data = (comm->get_type() == CommImplType::SEND ? comm->src_data_ : comm->dst_data_);
        return (comm->get_type() == type && (not match_fun || match_fun(this_user_data, other_user_data, comm.get())) &&
                (not comm->match_fun || comm->match_fun(other_user_data, this_user_data, my_synchro.get())));
      });
  if (iter == comm_queue.end()) {
    XBT_DEBUG("No matching communication synchro found");
    return nullptr;
  }

  const CommImplPtr& comm = *iter;
  XBT_DEBUG("Found a matching communication synchro %p", comm.get());
#if SIMGRID_HAVE_MC
  comm->mbox_cpy = comm->get_mailbox();
#endif
  comm->set_mailbox(nullptr);
  CommImplPtr comm_cpy = comm;
  if (remove_matching)
    comm_queue.erase(iter);
  return comm_cpy;
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
