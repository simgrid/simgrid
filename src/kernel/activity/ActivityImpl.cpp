/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"

simgrid::kernel::activity::ActivityImpl::ActivityImpl() = default;
simgrid::kernel::activity::ActivityImpl::~ActivityImpl() = default;

void simgrid::kernel::activity::ActivityImpl::ref()
{
  // Atomic operation! Do not split in two instructions!
  xbt_assert(refcount_ != 0);
  refcount_++;
}

void simgrid::kernel::activity::ActivityImpl::unref()
{
  xbt_assert(refcount_ > 0,
             "This activity has a negative refcount! You can only call test() or wait() once per activity.");

  // Atomic operation! Do not split in two instructions!
  auto count = --refcount_;
  if (count == 0)
    delete this;
}
