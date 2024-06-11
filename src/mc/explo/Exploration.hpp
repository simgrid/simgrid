/* Copyright (c) 2016-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include "simgrid/forward.h"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_record.hpp"
#include "xbt/asserts.h"
#include <xbt/Extendable.hpp>

#include <memory>

namespace simgrid::mc {

/** A model-checking exploration algorithm
 *
 *  This is an abstract base class used to group the data, state, configuration
 *  of a model-checking algorithm.
 *
 *  Implementing this interface will probably not be really mandatory,
 *  you might be able to write your model-checking algorithm as plain
 *  imperative code instead.
 *
 *  It is expected to interact with the model-checked application through the
 * `RemoteApp` interface (that is currently not perfectly sufficient to that extend). */
// abstract
class Exploration : public xbt::Extendable<Exploration> {
  std::unique_ptr<RemoteApp> remote_app_;
  static Exploration* instance_;

  FILE* dot_output_ = nullptr;
  int errors_       = 0; // Amount of errors seen so far; tested against model-check/max-errors

  /** @brief Wether the current exploration is a CriticalTransitionExplorer */
  bool is_looking_for_critical = false;

protected:
  unsigned long backtrack_count_      = 0; // for statistics
  unsigned long visited_states_count_ = 0; // for statistics

  time_t starting_time_; // For timeouts

public:
  explicit Exploration(const std::vector<char*>& args);
  explicit Exploration(std::unique_ptr<RemoteApp> remote_app);
  virtual ~Exploration();

  static Exploration* get_instance() { return instance_; }
  // No copy:
  Exploration(Exploration const&)            = delete;
  Exploration& operator=(Exploration const&) = delete;

  /** Main function of this algorithm */
  virtual void run() = 0;

  /** Allows the exploration to report that it found a correct execution. This updates a bool informations
   *  in corresponding states, and eventually rise an exception if we were looking for the critical transition. */
  void report_correct_execution(State* last_state);

  /** Produce an error message indicating that the application crashed (status was produced by waitpid) */
  XBT_ATTRIB_NORETURN void report_crash(int status);
  /** Produce an error message indicating that a property was violated */
  XBT_ATTRIB_NORETURN void report_assertion_failure();
  /** Check whether the application is deadlocked, and report it if so */
  void check_deadlock();

  /** Returns the amount of deadlocks seen so far (if model-checker/max-deadlocks is not 0) */
  int errors_seen() const { return errors_; }

  /** Returns whether the soft timeout elapsed, asking to end the exploration at the next backtracking point */
  bool soft_timouted() const;

  bool is_critical_transition_explorer() const { return is_looking_for_critical; }

  /* sanity check returning true if there is no actor to run in the simulation */
  bool empty();

  /** @brief Restore the application to that state, by rollback of a previously saved fork and replay the transitions
   * afterward
   *
   * @param finalize_app wether we should try to finalize the app before rollbacking. This should always be done,
   *                     except if you already know that the application is dead.
   */
  void backtrack_to_state(State* target_state, bool finalize_app = true);

  /* These methods are callbacks called by the model-checking engine
   * to get and display information about the current state of the
   * model-checking algorithm: */

  /** Retrieve the current stack to build an execution trace */
  virtual RecordTrace get_record_trace() = 0;

  /** Generate a textual execution trace of the simulated application */
  std::vector<std::string> get_textual_trace(int max_elements = -1);

  /** Log additional information about the state of the model-checker */
  virtual void log_state();

  virtual stack_t get_stack()
  {
    xbt_die("You asked for a combination of feature that is not yet supported by SimgridMC (most likely a combination "
            "of exploration algorithm + another special feature). If you really want to try this combination, reach "
            "out to us so we can cover those.");
  }

  RemoteApp& get_remote_app() { return *remote_app_.get(); }

  static xbt::signal<void(State&, RemoteApp&)> on_restore_state_signal;
  static xbt::signal<void(Transition*, RemoteApp&)> on_transition_replay_signal;
  static xbt::signal<void(RemoteApp&)> on_backtracking_signal;

  /** Called when the state to which we backtrack was not checkpointed state, forcing us to restore the initial state
   * before replaying some transitions */
  static void on_restore_state(std::function<void(State& state, RemoteApp& remote_app)> const& f)
  {
    on_restore_state_signal.connect(f);
  }
  /** Called when replaying a transition that was previously executed, to reach a backtracked state */
  static void on_transition_replay(std::function<void(Transition*, RemoteApp& remote_app)> const& f)
  {
    on_transition_replay_signal.connect(f);
  }
  /** Called each time that the exploration backtracks from a exploration end */
  static void on_backtracking(std::function<void(RemoteApp& remote_app)> const& f)
  {
    on_backtracking_signal.connect(f);
  }

  /** @brief If the exploration is not already looking for a critical transition,
   *  start a critical transition explorer.
   *
   *
   *  @return If a critical exploration has already started, this function call returns
   *  false without any effect. Else, it starts a critical transition explorer, runs it,
   *  and returns true when it has finished.
   */

  bool try_to_launch_critical_exploration();

  /** Print something to the dot output file*/
  void dot_output(const char* fmt, ...) XBT_ATTRIB_PRINTF(2, 3);
};

// External constructors so that the types (and the types of their content) remain hidden
XBT_PUBLIC Exploration* create_dfs_exploration(const std::vector<char*>& args, ReductionMode mode);
XBT_PUBLIC Exploration* create_out_of_order_exploration(const std::vector<char*>& args, ReductionMode mode);
XBT_PUBLIC Exploration* create_critical_transition_exploration(std::unique_ptr<RemoteApp> remote_app,
                                                               ReductionMode mode, stack_t* stack);

XBT_PUBLIC Exploration* create_communication_determinism_checker(const std::vector<char*>& args, ReductionMode mode);
XBT_PUBLIC Exploration* create_udpor_checker(const std::vector<char*>& args);

} // namespace simgrid::mc

#endif
