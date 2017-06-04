/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::ActivityImpl()  = default;
ActivityImpl::~ActivityImpl() = default;

void ActivityImpl::ref()
{
  // Atomic operation! Do not split in two instructions!
  xbt_assert(refcount_ != 0);
  refcount_++;
}

void ActivityImpl::unref()
{
  xbt_assert(refcount_ > 0,
             "This activity has a negative refcount! You can only call test() or wait() once per activity.");
  refcount_--;
  if (refcount_ == 0)
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
