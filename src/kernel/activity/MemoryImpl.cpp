/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "MemoryImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "xbt/asserts.h"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_smo, kernel, "Mailbox implementation");

namespace simgrid::kernel::activity {

std::vector<std::unordered_map<void*, std::vector<std::pair<std::string, std::string>>>>
    MemoryAccessImpl::saved_accesses_ = {};

MemoryAccessImpl::MemoryAccessImpl(actor::ActorImpl* issuer) : issuer_(issuer) {}

void MemoryAccessImpl::record_memory_access(MemOpType type, void* where)
{

  memory_accesses_.emplace_back(type, where);

  // If we are in replay mode, save every backtrace for memory accesses so we
  // can display them in case of a datarace
  if (MC_record_replay_is_active()) {
    xbt::Backtrace backtrace;
    XBT_VERB("[AID:=%ld] %s for location %p in round %lu\n%s", issuer_->get_pid(),
             type == MemOpType::READ ? "READ" : "WRITE", where, curr_round_, backtrace.resolve().c_str());

    auto aid = issuer_->get_pid();
    xbt_assert(aid >= 0);
    if (saved_accesses_.size() <= (unsigned)aid)
      saved_accesses_.resize(aid + 1);

    if (saved_accesses_[aid][where].size() <= curr_round_) {
      saved_accesses_[aid][where].resize(curr_round_ + 1);
    }
    if (type == MemOpType::READ) {
      saved_accesses_[aid][where][curr_round_].first = backtrace.resolve();
    } else
      saved_accesses_[aid][where][curr_round_].second = backtrace.resolve();
  }
}

void MemoryAccessImpl::serialize(mc::Channel& channel)
{
  size_t size = memory_accesses_.size();
  channel.pack<unsigned long>(size);
  for (auto mem : memory_accesses_) {
    channel.pack<MemOpType>(mem.get_type());
    channel.pack<void*>(mem.get_location());
  }

  memory_accesses_.clear();
  curr_round_++;
}
std::string MemoryAccessImpl::get_info_from_access(aid_t aid, unsigned long round, MemoryAccess mem_op)
{
  xbt_assert(aid >= 0);
  xbt_assert(saved_accesses_.size() > (unsigned)aid);
  xbt_assert(saved_accesses_[aid].find(mem_op.get_location()) != saved_accesses_[aid].end());
  xbt_assert(saved_accesses_[aid][mem_op.get_location()].size() > round, "aid=%ld size=%zu", aid,
             saved_accesses_[aid][mem_op.get_location()].size());

  auto where_to_find = saved_accesses_[aid][mem_op.get_location()][round];
  if (mem_op.get_type() == MemOpType::READ)
    return where_to_find.first;
  else
    return where_to_find.second;
}
} // namespace simgrid::kernel::activity
