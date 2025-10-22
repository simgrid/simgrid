/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/sthread/sthread.h"
#include "xbt/asserts.h"

#include <condition_variable>
#include <mutex>

namespace simgrid::xbt {
class XBT_PUBLIC OsSemaphore {
public:
  explicit inline OsSemaphore(unsigned int capa) : capa_(capa)
  {
    xbt_assert(not sthread_is_enabled(),
               "SimGrid created a system semaphore while in sthread mode. SimGrid should not intercept itself.");
  }

  inline void acquire()
  {
    std::unique_lock lock(mutex_);
    condition_.wait(lock, [this]() { return capa_ > 0; });
    --capa_;
  }

  inline void release()
  {
    const std::scoped_lock lock(mutex_);
    ++capa_;
    condition_.notify_one();
  }
  unsigned int size() { return capa_; }

private:
  unsigned int capa_;
  std::mutex mutex_;
  std::condition_variable condition_;
};
} // namespace simgrid::xbt
