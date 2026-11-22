/* Copyright (c) 2025-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smemory_observer.h"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/smemory/MemoryAccessTrace.hpp"
#include "src/sthread/sthread.h"
#include "xbt/log.h"

#include <cerrno>
#include <cstdint>
#include <malloc.h>
#include <unordered_set>

using namespace simgrid::mc::smemory;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smemory);

// Whether we are inside the observer, which must not observe itself (it may allocate memory, for instance)
static bool in_observer = false;

static void create_memory_access(MemOpType type, uintptr_t where, uint64_t size)
{
  if (in_observer)
    return;
  in_observer = true;

  // Do not track data on a stack, there is no datarace there
  if (smemory_is_on_stack(where)) {
    XBT_DEBUG("%p is on stack, ignore it", (void*)where);
    in_observer = false;
    return;
  }

  // Do not track data on read-only segments
  if (not smemory_is_rw_segment(where)) {
    XBT_DEBUG("%p is on a read-only segment, ignore it", (void*)where);
    in_observer = false;
    return;
  }

  bool watched = false;
  for (auto a : get_mc_watch_addresses()) {
    if (a == where) {
      watched = 1;
      break;
    }
  }
  auto* issuer = simgrid::kernel::actor::ActorImpl::self();
  if (watched) {
    if (simgrid::mc::get_model_checking_mode() == simgrid::mc::ModelCheckingMode::REPLAY) {
      // The replay shows where the accesses were done when it reports a data race on them
      get_mc_watched_accesses().push_back(
          {issuer ? issuer->get_pid() : -1, type == MemOpType::Write, 0, simgrid::xbt::Backtrace()});
    } else {
      XBT_INFO("%s access on the watched variable %p", type == MemOpType::Read ? "READ" : "WRITE", (void*)where);
      xbt_backtrace_display_current();
    }
  }

  if (issuer)
    issuer->get_memory_trace()->create_memory_access(type, where, size);
  in_observer = false;
}

void __mcsimgrid_write(uintptr_t where, uint64_t size)
{
  create_memory_access(MemOpType::Write, where, size);
}
void __mcsimgrid_read(uintptr_t where, uint64_t size)
{
  create_memory_access(MemOpType::Read, where, size);
}

#ifdef __GLIBC__
/* The allocation functions are intercepted, as ThreadSanitizer does: a new block resets the shadow memory of its range
 * (nothing done to that memory before its allocation can race with what is done after), and freeing a block writes it
 * whole (to report the races between a free and the accesses of other actors). The functions of the glibc are called
 * directly, since dlsym() itself allocates memory.
 *
 * Only the blocks allocated by the application are observed: SimGrid allocates and frees memory in the actors too, out
 * of the application code (sthread disabled), and freeing these blocks must not look like a write of the actor. */
extern "C" {
void* __libc_malloc(size_t size);
void* __libc_calloc(size_t count, size_t size);
void* __libc_realloc(void* ptr, size_t size);
void __libc_free(void* ptr);
void* __libc_memalign(size_t alignment, size_t size);
}

// The blocks allocated by the application and not freed yet
static std::unordered_set<void*>& application_blocks()
{
  static std::unordered_set<void*> blocks;
  return blocks;
}

static void record_allocation(void* ptr, size_t size)
{
  if (ptr == nullptr || in_observer || not sthread_is_enabled())
    return;
  auto* issuer = simgrid::kernel::actor::ActorImpl::self();
  if (issuer == nullptr)
    return;
  in_observer = true;
  issuer->get_memory_trace()->create_allocation(reinterpret_cast<uintptr_t>(ptr), size);
  application_blocks().insert(ptr);
  in_observer = false;
}

static void record_free(void* ptr)
{
  if (ptr == nullptr || in_observer)
    return;
  in_observer      = true;
  bool application = application_blocks().erase(ptr) > 0;
  in_observer      = false;
  if (application)
    create_memory_access(MemOpType::Write, reinterpret_cast<uintptr_t>(ptr), malloc_usable_size(ptr));
}

void* malloc(size_t size)
{
  void* res = __libc_malloc(size);
  record_allocation(res, size);
  return res;
}
void* calloc(size_t count, size_t size)
{
  void* res = __libc_calloc(count, size);
  record_allocation(res, count * size);
  return res;
}
void* realloc(void* ptr, size_t size)
{
  record_free(ptr); // The old block may be moved: consider it freed
  void* res = __libc_realloc(ptr, size);
  record_allocation(res, size);
  return res;
}
void free(void* ptr)
{
  record_free(ptr);
  __libc_free(ptr);
}
void* memalign(size_t alignment, size_t size)
{
  void* res = __libc_memalign(alignment, size);
  record_allocation(res, size);
  return res;
}
void* aligned_alloc(size_t alignment, size_t size)
{
  return memalign(alignment, size);
}
int posix_memalign(void** res, size_t alignment, size_t size)
{
  *res = memalign(alignment, size);
  return *res == nullptr ? ENOMEM : 0;
}
#endif
