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
    sthread_pause_guard guard; // Put sthread on pause during this call on need
    mutex_     = new std::mutex();
    condition_ = new std::condition_variable();
  }
  ~OsSemaphore()
  {
    sthread_pause_guard guard; // Put sthread on pause during this call on need
    delete condition_;
    delete mutex_;
  }

  inline void acquire()
  {
    sthread_pause_guard guard; // Put sthread on pause during this call on need
    std::unique_lock lock(*mutex_);
    condition_->wait(lock, [this]() { return capa_ > 0; });
    --capa_;
  }

  inline void release()
  {
    sthread_pause_guard guard; // Put sthread on pause during this call on need
    const std::scoped_lock lock(*mutex_);
    ++capa_;
    condition_->notify_one();
  }
  unsigned int size() { return capa_; }

private:
  unsigned int capa_;
  std::mutex* mutex_;
  std::condition_variable* condition_;
};
} // namespace simgrid::xbt
