/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::~ActivityImpl()
{
  if (surf_action_) {
    surf_action_->unref();
    XBT_DEBUG("Destroy activity %p", this);
    surf_action_ = nullptr;
  }
}

void ActivityImpl::suspend()
{
  if (surf_action_ == nullptr)
    return;
  XBT_VERB("This activity is suspended (remain: %f)", surf_action_->get_remains());
  surf_action_->suspend();
  on_suspended(*this);
}

void ActivityImpl::resume()
{
  if (surf_action_ == nullptr)
    return;
  XBT_VERB("This activity is resumed (remain: %f)", surf_action_->get_remains());
  surf_action_->resume();
  on_resumed(*this);
}

void ActivityImpl::set_category(const std::string& category)
{
  if (surf_action_)
    surf_action_->set_category(category);
}

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ActivityImpl* activity)
{
  activity->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(simgrid::kernel::activity::ActivityImpl* activity)
{
  if (activity->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete activity;
  }
}
xbt::signal<void(ActivityImpl const&)> ActivityImpl::on_resumed;
xbt::signal<void(ActivityImpl const&)> ActivityImpl::on_suspended;
}
}
} // namespace simgrid::kernel::activity::
