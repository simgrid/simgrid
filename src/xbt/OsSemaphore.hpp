/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include <condition_variable>
#include <mutex>

namespace simgrid::xbt {
class XBT_PUBLIC OsSemaphore {
public:
  explicit inline OsSemaphore(unsigned int capa) : capa_(capa) {}

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

private:
  unsigned int capa_;
  std::mutex mutex_;
  std::condition_variable condition_;
};
} // namespace simgrid::xbt
