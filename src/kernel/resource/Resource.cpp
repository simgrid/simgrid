/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Resource.hpp"
#include "src/kernel/lmm/maxmin.hpp" // Constraint
#include "src/surf/surf_interface.hpp"
#include "src/surf/trace_mgr.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

Resource::Resource(Model* model, const std::string& name, lmm::Constraint* constraint)
    : name_(name), model_(model), constraint_(constraint)
{
}

Resource::~Resource() = default;

bool Resource::is_on() const
{
  return is_on_;
}
bool Resource::is_off() const
{
  return not is_on_;
}

void Resource::turn_on()
{
  is_on_ = true;
}

void Resource::turn_off()
{
  is_on_ = false;
}

double Resource::get_load()
{
  return constraint_->get_usage();
}

Model* Resource::get_model() const
{
  return model_;
}

const std::string& Resource::get_name() const
{
  return name_;
}

const char* Resource::get_cname() const
{
  return name_.c_str();
}

bool Resource::operator==(const Resource& other) const
{
  return name_ == other.name_;
}

void Resource::set_state_trace(tmgr_trace_t trace)
{
  xbt_assert(state_event_ == nullptr, "Cannot set a second state trace to %s", get_cname());

  state_event_ = future_evt_set.add_trace(trace, this);
}

kernel::lmm::Constraint* Resource::get_constraint() const
{
  return constraint_;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
