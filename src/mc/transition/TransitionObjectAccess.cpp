/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <xbt/string.hpp>

namespace simgrid::mc {

ObjectAccessTransition::ObjectAccessTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::OBJECT_ACCESS, issuer, times_considered)
{
  short s;
  xbt_assert(stream >> s >> objaddr_ >> objname_ >> file_ >> line_);
  access_type_ = static_cast<simgrid::mc::ObjectAccessType>(s);
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

  if (const auto* other = dynamic_cast<const ObjectAccessTransition*>(o))
    return objaddr_ == other->objaddr_; // dependent only if it's an access to the same object
  return false;
}

bool ObjectAccessTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::OBJECT_ACCESS, "Unexpected transition type %s", to_c_str(type_));

  return true; // Object access is always enabled
}

} // namespace simgrid::mc
