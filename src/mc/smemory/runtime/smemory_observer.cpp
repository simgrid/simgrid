/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smemory_observer.h"
#include "src/kernel/activity/MemoryImpl.hpp"

void create_memory_access(MemOpType, void*, unsigned char size);

static bool instrument = true;

void create_memory_access(MemOpType type, void* where, unsigned char size)
{
  // Break the recursion: we don't want to instrument the instrumenter
  if (not instrument)
    return;

  instrument                                = false;
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  if (issuer && issuer->get_memory_access())
    issuer->get_memory_access()->record_memory_access(type, where);
  instrument = true;
}

void __mcsimgrid_write(void* where, unsigned char size)
{
  create_memory_access(MemOpType::WRITE, where, size);
}
void __mcsimgrid_read(void* where, unsigned char size)
{
  create_memory_access(MemOpType::READ, where, size);
}
