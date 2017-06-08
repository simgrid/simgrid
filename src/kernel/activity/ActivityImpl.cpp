/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

XBT_LOG_EXTERNAL_CATEGORY(simix_network);

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::ActivityImpl()  = default;
ActivityImpl::~ActivityImpl() = default;

void ActivityImpl::ref()
{
  xbt_assert(refcount_ >= 0);
  refcount_++;
  XBT_CDEBUG(simix_network, "%p->refcount++ ~> %d", this, (int)refcount_);
  if (XBT_LOG_ISENABLED(simix_network, xbt_log_priority_trace))
    xbt_backtrace_display_current();
}

void ActivityImpl::unref()
{
  XBT_CDEBUG(simix_network, "%p->refcount-- ~> %d", this, ((int)refcount_) - 1);
  xbt_assert(refcount_ >= 0);
  refcount_--;
  if (XBT_LOG_ISENABLED(simix_network, xbt_log_priority_trace))
    xbt_backtrace_display_current();
  if (refcount_ <= 0)
    delete this;
}

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ActivityImpl* activity)
{
  activity->ref();
}

void intrusive_ptr_release(simgrid::kernel::activity::ActivityImpl* activity)
{
  activity->unref();
}
}
}
} // namespace simgrid::kernel::activity::
