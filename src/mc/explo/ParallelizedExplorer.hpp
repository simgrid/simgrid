/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_CHECKER_HPP
#define SIMGRID_MC_SAFETY_CHECKER_HPP

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"

#include <algorithm>
#include <boost/lockfree/queue.hpp>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// template <typename T> using parallel_channel = boost::lockfree::queue<T, boost::lockfree::capacity<65534>>;

namespace simgrid::mc {

template <typename T> class ParallelChannel {

public:
  ParallelChannel()    = default;
  virtual T pop()      = 0;
  virtual void push(T) = 0;
  virtual void sort()  = 0;
};

template <typename T> class lock_free_channel : public ParallelChannel<T> {
  boost::lockfree::queue<T, boost::lockfree::capacity<65534>> queue_;
  // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
public:
  lock_free_channel() = default;
  T pop() override
  {
    T element;
    while (!queue_.pop(element)) {
    }
    return element;
  }
  void push(T element) override
  {
    while (!queue_.push(element)) {
    }
  }
  void sort() override
  {
    // This can't be done here, simply pass
    return;
  }
};

template <typename T> class mutex_channel : public ParallelChannel<T> {
  std::deque<T> queue_;
  std::mutex m_;

public:
  mutex_channel() = default;
  T pop() override
  {
    T element;
    while (true) {
      m_.lock();
      if (queue_.size() == 0) {
        m_.unlock();
        continue;
      } else {
        element = queue_.front();
        queue_.pop_front();
        m_.unlock();
        break;
      }
    }
    return element;
  }
  void push(T element) override
  {
    m_.lock();
    queue_.push_front(element);
    m_.unlock();
  }
  void sort() override
  {
    m_.lock();
    std::sort(queue_.begin(), queue_.end());
    m_.unlock();
  }
};

class ThreadLocalExplorer {
  const int explorer_id;
  std::unique_ptr<RemoteApp> remote_app_;

  static int explorer_count;

public:
  ThreadLocalExplorer(const std::vector<char*>& args)
      : explorer_id(explorer_count++)
      , remote_app_(std::make_unique<RemoteApp>(args, "thread" + std::to_string(explorer_id)))
  {
  }

  void check_deadlock();
  void backtrack_to_state(State* to_visit);

  stack_t stack;
  odpor::Execution execution_seq;

  RemoteApp& get_remote_app() { return *remote_app_.get(); }
  int get_explorer_id() const { return explorer_id; }

  unsigned long explored_traces      = 0; // for statistics
  unsigned long visited_states_count = 0; // for statistics

  Reduction* reduction_algo;
  lock_free_channel<State*>* opened_heads;
  lock_free_channel<Reduction::RaceUpdate*>* races_list;
};

class XBT_PRIVATE ParallelizedExplorer : public Exploration {

private:
  ReductionMode reduction_mode_;
  std::unique_ptr<Reduction> reduction_algo_;

  lock_free_channel<State*> opened_heads_;
  lock_free_channel<Reduction::RaceUpdate*> races_list_;

  std::vector<std::thread> thread_pool_;
  std::vector<std::shared_ptr<ThreadLocalExplorer>> local_explorers_;
  std::vector<ExitStatus> thread_results_;

  const std::vector<char*>& args_; // Application args saved to create threads

  int number_of_threads = _sg_mc_parallel_thread;

public:
  explicit ParallelizedExplorer(const std::vector<char*>& args, ReductionMode mode);
  void run() override;
  RecordTrace get_record_trace() override;
  void TreeHandler();
};

// Wrapper function around explorer that helps handling the different termination cases of the explorer
void ExplorerHandler( // const std::vector<char*>& args, Reduction* reduction_algo_,
                      // parallel_channel<State*>* opened_heads_, parallel_channel<Reduction::RaceUpdate*>* races_list_,
    ThreadLocalExplorer* local_explorer, ExitStatus* exploration_result);

void Explorer(ThreadLocalExplorer&);
RecordTrace get_record_trace_from_stack(stack_t& stack);

} // namespace simgrid::mc

#endif
