/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::ActivityImpl()  = default;
ActivityImpl::~ActivityImpl() = default;

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ActivityImpl* activity)
{
  activity->refcount_++;
}

void intrusive_ptr_release(simgrid::kernel::activity::ActivityImpl* activity)
{
  if (activity->refcount_-- == 0)
    delete activity;
}
}
}
} // namespace simgrid::kernel::activity::
