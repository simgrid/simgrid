/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include <memory>
#include <xbt/string.hpp>

namespace simgrid::mc {

ObjectAccessTransition::ObjectAccessTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::OBJECT_ACCESS, issuer, times_considered)
{
  data_               = std::make_unique<data>();
  data_->access_type_ = static_cast<simgrid::mc::ObjectAccessType>(channel.unpack<short>());
  data_->objaddr_     = channel.unpack<void*>();
  data_->objname_     = channel.unpack<std::string>();
  data_->file_        = channel.unpack<std::string>();
  data_->line_        = channel.unpack<int>();
}
std::string ObjectAccessTransition::to_string(bool verbose) const
{
  std::string res;
  if (data_->access_type_ == ObjectAccessType::ENTER)
    res = std::string("BeginObjectAccess(");
  else if (data_->access_type_ == ObjectAccessType::EXIT)
    res = std::string("EndObjectAccess(");
  else
    res = std::string("ObjectAccess(");
  res += data_->objname_;
  if (not xbt_log_no_loc)
    res += std::string(" @ ") + data_->file_ + ":" + std::to_string(data_->line_);
  res += std::string(")");
  return res;
}

bool ObjectAccessTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                             EventHandle this_handle, EventHandle other_handle) const
{
  xbt_assert(type_ == Type::OBJECT_ACCESS, "Unexpected transition type %s", to_c_str(type_));

  return true; // Object access is always enabled
}

} // namespace simgrid::mc
