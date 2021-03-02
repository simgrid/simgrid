/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/SimcallInspector.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_inspector, mc, "Logging specific to MC simcall inspection");

namespace simgrid {
namespace mc {

std::string SimcallInspector::to_string(int /*time_considered*/) const
{
  return simgrid::xbt::string_printf("[(%ld)%s (%s)] ", get_issuer()->get_pid(), issuer_->get_host()->get_cname(),
                                     issuer_->get_cname());
}

std::string RandomSimcall::to_string(int time_considered) const
{
  return SimcallInspector::to_string(time_considered) + "MC_RANDOM(" + std::to_string(time_considered) + ")";
}

std::string SimcallInspector::dot_label() const
{
  if (issuer_->get_host())
    return xbt::string_printf("[(%ld)%s]", issuer_->get_pid(), issuer_->get_cname());
  return xbt::string_printf("[(%ld)]", issuer_->get_pid());
}

std::string RandomSimcall::dot_label() const
{
  return SimcallInspector::dot_label() + " MC_RANDOM (" + std::to_string(next_value_) + ")";
}

void RandomSimcall::prepare(int times_considered)
{
  next_value_ = min_ + times_considered;
  XBT_DEBUG("MC_RANDOM(%d, %d) will return %d after %d times", min_, max_, next_value_, times_considered);
}

int RandomSimcall::get_max_consider() const
{
  return max_ - min_ + 1;
}

int RandomSimcall::get_value() const
{
  return next_value_;
}

} // namespace mc
} // namespace simgrid
