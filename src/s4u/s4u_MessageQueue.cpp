/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Mess.hpp>
#include <simgrid/s4u/MessageQueue.hpp>

#include "src/kernel/activity/MessageQueueImpl.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_mqueue, s4u, "S4U Message Queues");

namespace simgrid::s4u {

const std::string& MessageQueue::get_name() const
{
  return pimpl_->get_name();
}

const char* MessageQueue::get_cname() const
{
  return pimpl_->get_cname();
}

MessageQueue* MessageQueue::by_name(const std::string& name)
{
  return Engine::get_instance()->message_queue_by_name_or_create(name);
}

bool MessageQueue::empty() const
{
  return pimpl_->empty();
}

size_t MessageQueue::size() const
{
  return pimpl_->size();
}

kernel::activity::MessImplPtr MessageQueue::front() const
{
  return pimpl_->empty() ? nullptr : pimpl_->front();
}

MessPtr MessageQueue::put_init()
{
  MessPtr res(new Mess());
  res->set_queue(this);
  res->sender_ = kernel::actor::ActorImpl::self();
  return res;
}

MessPtr MessageQueue::put_init(void* payload)
{
  return put_init()->set_payload(payload);
}

MessPtr MessageQueue::put_async(void* payload)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");
  MessPtr res = put_init(payload);
  res->start();
  return res;
}

void MessageQueue::put(void* payload)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");

  put_async(payload)->wait();
}

/** Blocking send with timeout */
void MessageQueue::put(void* payload, double timeout)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");

  put_init()->set_payload(payload)->start()->wait_for(timeout);
}

MessPtr MessageQueue::get_init()
{
  MessPtr res(new Mess());
  res->set_queue(this);
  res->receiver_ = kernel::actor::ActorImpl::self();
  return res;
}

MessPtr MessageQueue::get_async()
{
  MessPtr res = get_init()->set_payload(nullptr);
  res->start();
  return res;
}

} // namespace simgrid::s4u
