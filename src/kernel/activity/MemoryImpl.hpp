/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP
#define SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/utility.hpp"
#include <boost/intrusive/list.hpp>
#include <cstdio>
#include <string>
#include <unordered_map>

XBT_DECLARE_ENUM_CLASS(MemOpType, READ = 0, WRITE = 1);

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

  // Memorry accesses made by the actor
  std::vector<activity::MemoryAccess> memory_accesses_;

  // Used to save backtrace in replay mode. This is mandatory if we want to display them for
  // the user when the race is finally detected.
  // This has to be static since the actor can die before we detect the corresponding race
  // vecotr[aid is key] -> Map location -> vector[round is key] -> <Read, Write> data
  static std::vector<std::unordered_map<void*, std::vector<std::pair<std::string, std::string>>>> saved_accesses_;

  unsigned long curr_round_ = 0;

public:
  MemoryAccessImpl(actor::ActorImpl* issuer);

  // Called by the user code
  void record_memory_access(MemOpType type, void* where);

  actor::ActorImpl* get_issuer() const { return issuer_; }

  // Retrieve the value saved in the static structure
  static std::string get_info_from_access(aid_t aid, unsigned long round, MemoryAccess mem_op);

  // Prepare the data corresponding to the savec memory accesses and pack them in the channel.
  // After that, flush the data so we are ready to register the next round.
  void serialize(mc::Channel& channel);

  void finish() override {}
};
} // namespace simgrid::kernel::activity

#endif // SIMGRID_KERNEL_ACTIVITY_MEMORY_HPP
