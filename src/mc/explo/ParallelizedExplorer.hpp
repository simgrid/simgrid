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

#include <boost/lockfree/queue.hpp>
#include <condition_variable>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

template <typename T> using parallel_channel = boost::lockfree::queue<T, boost::lockfree::capacity<1024>>;

namespace simgrid::mc {

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
  parallel_channel<State*>* opened_heads;
  parallel_channel<Reduction::RaceUpdate*>* races_list;
};

class XBT_PRIVATE ParallelizedExplorer : public Exploration {

private:
  ReductionMode reduction_mode_;
  std::unique_ptr<Reduction> reduction_algo_;

  parallel_channel<State*> opened_heads_;
  parallel_channel<Reduction::RaceUpdate*> races_list_;

  std::vector<std::thread> thread_pool_;
  std::vector<std::shared_ptr<ThreadLocalExplorer>> local_explorers_;
  std::vector<ExitStatus> thread_results_;

  const std::vector<char*>& args_; // Application args saved to create threads

  int number_of_threads = 2;

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
