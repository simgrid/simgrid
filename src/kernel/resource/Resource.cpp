/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Resource.hpp"
#include "src/kernel/lmm/maxmin.hpp" // Constraint
#include "src/surf/surf_interface.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

Resource::Resource(Model* model, const std::string& name, lmm::Constraint* constraint)
    : name_(name), model_(model), constraint_(constraint)
{
}

Resource::~Resource() = default;

bool Resource::isOn() const
{
  return isOn_;
}
bool Resource::isOff() const
{
  return not isOn_;
}

void Resource::turnOn()
{
  isOn_ = true;
}

void Resource::turnOff()
{
  isOn_ = false;
}

double Resource::getLoad()
{
  return constraint_->get_usage();
}

Model* Resource::model() const
{
  return model_;
}

const std::string& Resource::getName() const
{
  return name_;
}

const char* Resource::getCname() const
{
  return name_.c_str();
}

bool Resource::operator==(const Resource& other) const
{
  return name_ == other.name_;
}

kernel::lmm::Constraint* Resource::constraint() const
{
  return constraint_;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
