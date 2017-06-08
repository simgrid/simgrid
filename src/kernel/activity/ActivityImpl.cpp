/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

XBT_LOG_EXTERNAL_CATEGORY(simix_process);

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::ActivityImpl()  = default;
ActivityImpl::~ActivityImpl() = default;

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ActivityImpl* activity)
{
  xbt_assert(activity->refcount_ >= 0);
  activity->refcount_++;
  XBT_CDEBUG(simix_process, "%p->refcount++ ~> %d", activity, (int)activity->refcount_);
  if (XBT_LOG_ISENABLED(simix_process, xbt_log_priority_trace))
    xbt_backtrace_display_current();
}

void intrusive_ptr_release(simgrid::kernel::activity::ActivityImpl* activity)
{
  XBT_CDEBUG(simix_process, "%p->refcount-- ~> %d", activity, ((int)activity->refcount_) - 1);
  xbt_assert(activity->refcount_ >= 0);
  activity->refcount_--;
  if (XBT_LOG_ISENABLED(simix_process, xbt_log_priority_trace))
    xbt_backtrace_display_current();
  if (activity->refcount_ <= 0)
    delete activity;
}
}
}
} // namespace simgrid::kernel::activity::
