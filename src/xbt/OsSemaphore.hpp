/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <condition_variable>
#include <mutex>

namespace simgrid {
namespace xbt {
class XBT_PUBLIC OsSemaphore {
public:
  inline OsSemaphore(unsigned int uiCount) : capa_(uiCount) {}

  inline void acquire()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&]() -> bool { return capa_ > 0; });
    --capa_;
  }

  inline void release()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    ++capa_;
    condition_.notify_one();
  }

private:
  unsigned int capa_;
  std::mutex mutex_;
  std::condition_variable condition_;
};
} // namespace xbt
} // namespace simgrid
