/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MessageQueueImpl.hpp"

#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_mq, kernel, "Message queue implementation");

namespace simgrid::kernel::activity {

unsigned MessageQueueImpl::next_id_ = 0;

MessageQueueImpl::~MessageQueueImpl()
{
  try {
    clear();
  } catch (const std::bad_alloc& ba) {
    XBT_ERROR("MessageQueueImpl::clear() failure: %s", ba.what());
  }
}

/** @brief Removes all message activities from a message queue */
void MessageQueueImpl::clear()
{
  while (not queue_.empty()) {
    auto mess = queue_.back();
    if (mess->get_state() == State::WAITING) {
      mess->cancel();
      mess->set_state(State::FAILED);
    } else
      queue_.pop_back();
  }
  xbt_assert(queue_.empty());
}

void MessageQueueImpl::push(const MessImplPtr& mess)
{
  mess->set_queue(this);
  this->queue_.push_back(std::move(mess));
}

void MessageQueueImpl::remove(const MessImplPtr& mess)
{
  xbt_assert(mess->get_queue() == this, "Message %p is in queue %s, not queue %s", mess.get(),
             (mess->get_queue() ? mess->get_queue()->get_cname() : "(null)"), get_cname());

  mess->set_queue(nullptr);
  auto it = std::find(queue_.begin(), queue_.end(), mess);
  if (it != queue_.end())
    queue_.erase(it);
  else
    xbt_die("Message %p not found in queue %s", mess.get(), get_cname());
}

MessImplPtr MessageQueueImpl::find_matching_message(MessImplType type)
{
  auto iter = std::find_if(queue_.begin(), queue_.end(), [&type](const MessImplPtr& mess)
  {
    return (mess->get_type() == type);
  });
  if (iter == queue_.end()) {
    XBT_DEBUG("No matching message synchro found");
    return nullptr;
  }

  const MessImplPtr& mess = *iter;
  XBT_DEBUG("Found a matching message synchro %p", mess.get());
  mess->set_queue(nullptr);
  MessImplPtr mess_cpy = mess;
  queue_.erase(iter);
  return mess_cpy;
}

} // namespace simgrid::kernel::activity
