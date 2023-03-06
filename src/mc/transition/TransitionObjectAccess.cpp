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
  type_ = static_cast<simgrid::mc::ObjectAccessType>(s);
}
std::string ObjectAccessTransition::to_string(bool verbose) const
{
  if (type_ == ObjectAccessType::ENTER)
    return xbt::string_printf("BeginObjectAccess(%s @ %s:%d)", objname_.c_str(), file_.c_str(), line_);
  if (type_ == ObjectAccessType::EXIT)
    return xbt::string_printf("EndObjectAccess(%s @ %s:%d)", objname_.c_str(), file_.c_str(), line_);
  return xbt::string_printf("ObjectAccess(%s @ %s:%d)", objname_.c_str(), file_.c_str(), line_);
}
bool ObjectAccessTransition::depends(const Transition* o) const
{
  if (const auto* other = dynamic_cast<const ObjectAccessTransition*>(o))
    return objaddr_ == other->objaddr_; // dependent only if it's an access to the same object
  return false;
}

} // namespace simgrid::mc
