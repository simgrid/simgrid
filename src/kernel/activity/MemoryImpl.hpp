/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP
#define SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/utility.hpp"
#include <boost/intrusive/list.hpp>
#include <cstdio>
#include <string>

XBT_DECLARE_ENUM_CLASS(MemOpType, READ, WRITE);

namespace simgrid::kernel::activity {

class MemoryAccess {
  MemOpType type_;
  void* where_;

public:
  MemoryAccess(MemOpType type, void* where) : type_(type), where_(where){};
  void* get_location() const { return where_; }
  MemOpType get_type() const { return type_; }
};

class XBT_PUBLIC MemoryAccessImpl : public ActivityImpl {
  actor::ActorImpl* issuer_ = nullptr;

  std::vector<activity::MemoryAccess> memory_accesses_;

public:
  MemoryAccessImpl(actor::ActorImpl* issuer);

  void record_memory_access(MemOpType type, void* where);

  actor::ActorImpl* get_issuer() const { return issuer_; }

  void serialize(mc::Channel& channel);

  void finish() override {}
};
} // namespace simgrid::kernel::activity

#endif // SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP
