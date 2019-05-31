/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Resource.hpp"
#include "src/kernel/lmm/maxmin.hpp" // Constraint
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/surf_interface.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

Resource::~Resource() = default;

double Resource::get_load() const
{
  return constraint_->get_usage();
}

void Resource::set_state_profile(profile::Profile* profile)
{
  xbt_assert(state_event_ == nullptr, "Cannot set a second state profile to %s", get_cname());
  state_event_ = profile->schedule(&profile::future_evt_set, this);
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
