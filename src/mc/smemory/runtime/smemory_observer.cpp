/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smemory_observer.h"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/smemory/MemoryAccessTracker.hpp"
#include "xbt/log.h"

using namespace simgrid::mc::smemory;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smemory);

static void create_memory_access(MemOpType type, void* where, unsigned char size)
{
  static bool instrument = true;

  // Break the recursion: we don't want to instrument the instrumenter
  if (not instrument)
    return;
  instrument = false;

  // Do not track data on a stack, there is no datarace there
  if (smemory_is_on_stack(where)) {
    XBT_DEBUG("%p is on stack, ignore it", where);
    instrument = true;
    return;
  }

  bool watched = false;
  for (void* a : get_mc_watch_addresses()) {
    if (a == where) {
      watched = 1;
      break;
    }
  }
  if (watched) {
    XBT_INFO("%s access on the watched variable %p", type == MemOpType::Read ? "READ" : "WRITE", where);
    xbt_backtrace_display_current();
  }

  auto* issuer = simgrid::kernel::actor::ActorImpl::self();
  if (issuer)
    issuer->get_memory_tracker()->create_memory_access(type, where, size);
  instrument = true;
}

void __mcsimgrid_write(void* where, unsigned char size)
{
  create_memory_access(MemOpType::Write, where, size);
}
void __mcsimgrid_read(void* where, unsigned char size)
{
  create_memory_access(MemOpType::Read, where, size);
}
