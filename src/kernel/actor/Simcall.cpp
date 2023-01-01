/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/Simcall.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "xbt/log.h"

namespace simgrid::kernel::actor {

/** @brief returns a printable string representing a simcall */
const char* Simcall::get_cname() const
{
  if (observer_ != nullptr) {
    static std::string name;
    name              = boost::core::demangle(typeid(*observer_).name());
    const char* cname = name.c_str();
    if (name.rfind("simgrid::kernel::", 0) == 0)
      cname += 17; // strip prefix "simgrid::kernel::"
    return cname;
  } else {
    return to_c_str(call_);
  }
}
ObjectAccessSimcallItem::ObjectAccessSimcallItem()
{
  take_ownership();
}
void ObjectAccessSimcallItem::take_ownership()
{
  simcall_owner_ = ActorImpl::self();
}

} // namespace simgrid::kernel::actor
