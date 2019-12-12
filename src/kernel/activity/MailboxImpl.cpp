/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"

#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_mailbox, simix, "Mailbox implementation");

static std::unordered_map<std::string, smx_mailbox_t> mailboxes;

void SIMIX_mailbox_exit()
{
  for (auto const& elm : mailboxes)
    delete elm.second;
  mailboxes.clear();
}

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

namespace simgrid {
namespace kernel {
namespace activity {
/** @brief Returns the mailbox of that name, or nullptr */
MailboxImpl* MailboxImpl::by_name_or_null(const std::string& name)
{
  auto mbox = mailboxes.find(name);
  if (mbox != mailboxes.end())
    return mbox->second;
  else
    return nullptr;
}

/** @brief Returns the mailbox of that name, newly created on need */
MailboxImpl* MailboxImpl::by_name_or_create(const std::string& name)
{
  /* two processes may have pushed the same mbox_create simcall at the same time */
  auto m = mailboxes.find(name);
  if (m == mailboxes.end()) {
    MailboxImpl* mbox = new MailboxImpl(name);
    XBT_DEBUG("Creating a mailbox at %p with name %s", mbox, name.c_str());
    mailboxes[name] = mbox;
    return mbox;
  } else
    return m->second;
}
/** @brief set the receiver of the mailbox to allow eager sends
 *  @param actor The receiving dude
 */
void MailboxImpl::set_receiver(s4u::ActorPtr actor)
{
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

CommImplPtr MailboxImpl::iprobe(int type, bool (*match_fun)(void*, void*, CommImpl*), void* data)
{
  XBT_DEBUG("iprobe from %p %p", this, &comm_queue_);

  CommImplPtr this_comm;
  CommImpl::Type smx_type;
  if (type == 1) {
    this_comm = CommImplPtr(new CommImpl());
    this_comm->set_type(CommImpl::Type::SEND);
    smx_type  = CommImpl::Type::RECEIVE;
  } else {
    this_comm = CommImplPtr(new CommImpl());
    this_comm->set_type(CommImpl::Type::RECEIVE);
    smx_type  = CommImpl::Type::SEND;
  }
  CommImplPtr other_comm = nullptr;
  if (permanent_receiver_ != nullptr && not done_comm_queue_.empty()) {
    XBT_DEBUG("first check in the permanent recv mailbox, to see if we already got something");
    other_comm = find_matching_comm(smx_type, match_fun, data, this_comm, /*done*/ true, /*remove_matching*/ false);
  }
  if (not other_comm) {
    XBT_DEBUG("check if we have more luck in the normal mailbox");
    other_comm = find_matching_comm(smx_type, match_fun, data, this_comm, /*done*/ false, /*remove_matching*/ false);
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
CommImplPtr MailboxImpl::find_matching_comm(CommImpl::Type type, bool (*match_fun)(void*, void*, CommImpl*),
                                            void* this_user_data, const CommImplPtr& my_synchro, bool done,
                                            bool remove_matching)
{
  void* other_user_data = nullptr;
  auto& comm_queue      = done ? done_comm_queue_ : comm_queue_;

  for (auto it = comm_queue.begin(); it != comm_queue.end(); it++) {
    CommImplPtr& comm = *it;

    if (comm->type_ == CommImpl::Type::SEND) {
      other_user_data = comm->src_data_;
    } else if (comm->type_ == CommImpl::Type::RECEIVE) {
      other_user_data = comm->dst_data_;
    }
    if (comm->type_ == type && (match_fun == nullptr || match_fun(this_user_data, other_user_data, comm.get())) &&
        (not comm->match_fun || comm->match_fun(other_user_data, this_user_data, my_synchro.get()))) {
      XBT_DEBUG("Found a matching communication synchro %p", comm.get());
#if SIMGRID_HAVE_MC
      comm->mbox_cpy = comm->get_mailbox();
#endif
      comm->set_mailbox(nullptr);
      CommImplPtr comm_cpy = comm;
      if (remove_matching)
        comm_queue.erase(it);
      return comm_cpy;
    }
    XBT_DEBUG("Sorry, communication synchro %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              comm.get(), (int)comm->type_, (int)type);
  }
  XBT_DEBUG("No matching communication synchro found");
  return nullptr;
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
