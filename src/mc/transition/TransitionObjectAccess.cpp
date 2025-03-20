/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include <xbt/string.hpp>

namespace simgrid::mc {

ObjectAccessTransition::ObjectAccessTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::OBJECT_ACCESS, issuer, times_considered)
{
  access_type_ = static_cast<simgrid::mc::ObjectAccessType>(channel.unpack<short>());
  objaddr_     = channel.unpack<void*>();
  objname_     = channel.unpack<std::string>();
  file_        = channel.unpack<std::string>();
  line_        = channel.unpack<int>();
}
std::string ObjectAccessTransition::to_string(bool verbose) const
{
  std::string res;
  if (access_type_ == ObjectAccessType::ENTER)
    res = std::string("BeginObjectAccess(");
  else if (access_type_ == ObjectAccessType::EXIT)
    res = std::string("EndObjectAccess(");
  else
    res = std::string("ObjectAccess(");
  res += objname_;
  if (not xbt_log_no_loc)
    res += std::string(" @ ") + file_ + ":" + std::to_string(line_);
  res += std::string(")");
  return res;
}
bool ObjectAccessTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  if (o->type_ == Type::OBJECT_ACCESS) // dependent only if it's an access to the same object
    return objaddr_ == static_cast<const ObjectAccessTransition*>(o)->objaddr_;
  return false;
}

bool ObjectAccessTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                             EventHandle this_handle, EventHandle other_handle) const
{
  xbt_assert(type_ == Type::OBJECT_ACCESS, "Unexpected transition type %s", to_c_str(type_));

  return true; // Object access is always enabled
}

} // namespace simgrid::mc
