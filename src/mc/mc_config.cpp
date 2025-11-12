/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simgrid/sg_config.hpp"
#include "xbt/asserts.h"
#include "xbt/config.hpp"
#include <simgrid/modelchecker.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_cfg);

static simgrid::mc::ModelCheckingMode model_checking_mode = simgrid::mc::ModelCheckingMode::NONE;
simgrid::mc::ModelCheckingMode simgrid::mc::get_model_checking_mode()
{
  return model_checking_mode;
}
void simgrid::mc::set_model_checking_mode(simgrid::mc::ModelCheckingMode mode)
{
  model_checking_mode = mode;
}

static void _mc_cfg_cb_check(const char* spec, bool more_check = true)
{
  xbt_assert(_sg_cfg_init_status == 0 || MC_is_active() || MC_record_replay_is_active() || not more_check,
             "Specifying a %s is only allowed within the model-checker. Please use simgrid-mc, or specify this option "
             "after the replay path.",
             spec);
}

/* Replay (this part is enabled even if MC it disabled) */
simgrid::config::Flag<std::string> _sg_mc_record_path{
    "model-check/replay", "Model-check path to replay (as reported by SimGrid when a violation is reported)", "",
    [](std::string_view value) {
      if (value.empty()) // Ignore default value
        return;
      if (simgrid::mc::get_model_checking_mode() != simgrid::mc::ModelCheckingMode::NONE &&
          simgrid::mc::get_model_checking_mode() != simgrid::mc::ModelCheckingMode::REPLAY &&
          /* simgrid-mc will execve the binary to remove itself when this parameter is set */
          simgrid::mc::get_model_checking_mode() != simgrid::mc::ModelCheckingMode::CHECKER_SIDE)
        xbt_die("ERROR: Specifying a MC replay path is not allowed when running the model-checker in mode %s. "
                "Either remove the model-check/replay parameter, or execute your code out of simgrid-mc.",
                to_c_str(simgrid::mc::get_model_checking_mode()));
      simgrid::mc::set_model_checking_mode(simgrid::mc::ModelCheckingMode::REPLAY);
      MC_record_path() = value;
    }};

simgrid::config::Flag<bool> _sg_mc_timeout{
    "model-check/timeout", "Whether to enable timeouts for wait requests", false, [](bool) {
      _mc_cfg_cb_check("value to enable/disable timeout for wait requests", not MC_record_replay_is_active());
    }};

static simgrid::config::Flag<std::string> cfg_mc_reduction{
    "model-check/reduction", "Specify the kind of exploration reduction (none, DPOR, ODPOR or UDPOR)", "dpor",
    [](std::string_view value) {
      if (value != "none" && value != "dpor" && value != "sdpor" && value != "odpor" && value != "udpor")
        xbt_die("configuration option 'model-check/reduction' must be one of the following: "
                " 'dpor', 'sdpor', 'odpor', or 'udpor'");
    }};

simgrid::config::Flag<int> _sg_mc_cached_states_interval{
    "model-check/cached-states-interval",
    "Specify how often a state factory shall be created. This enables fast state restore at the cost of wasted memory.",
    1000,
    [](int val) { xbt_assert(val >= 0, "The value of model-check/cached-states-interval must be positive or null"); }};

simgrid::config::Flag<int> _sg_mc_parallel_thread{
    "model-check/parallel-thread",
    "Specify how explorer should be launched in parallel. Each explorer will be a different separate thread spawned by "
    "McSimGrid.",
    1, [](int val) { xbt_assert(val >= 1, "The value of model-check/parallel-thread must be >= 1"); }};

simgrid::config::Flag<std::string> _sg_mc_explore_algo{
    "model-check/exploration-algo",
    "Specify the type of search politic to use in MC algorithm",
    "DFS",
    {{"DFS",
      "Depth First Search order: this search politic is the one following the best the classical reduction algorithm."},
     {"BeFS",
      "Best First Search: this search politic allows for a better use of the strategy by augmenting the number of "
      "state choices available at runtime."},
     {"parallel",
      "parallel search: this work-in-progress aims at leveraging BeFS algorithm to heavily parallelize the search."}}};

simgrid::config::Flag<std::string> _sg_mc_strategy{
    "model-check/strategy",
    "Specify the kind of heuristic to use for guided model-checking",
    "none",
    {
        {"none", "No specific strategy: simply pick the first available transition and act as a DFS."},
        {"uniform", "No specific strategy: choices are made randomly based on a uniform sampling."},
    }};

simgrid::config::Flag<int> _sg_mc_random_seed{"model-check/rand-seed",
                                              "give a specific random seed to initialize the uniform distribution", 0,
                                              [](int) { _mc_cfg_cb_check("Random seed"); }};

simgrid::config::Flag<int> _sg_mc_k_alternatives{"model-check/k-alternatives",
                                                 "value of k to be used for k-alternatives UDPOR algorithm", -1,
                                                 [](int) { _mc_cfg_cb_check("k-Alternatives"); }};

simgrid::config::Flag<bool> _sg_mc_comms_determinism{
    "model-check/communications-determinism", "Whether to enable the detection of communication determinism", false,
    [](bool) {
      _mc_cfg_cb_check("value to enable/disable the detection of determinism in the communications schemes");
    }};
simgrid::config::Flag<bool> _sg_mc_send_determinism{
    "model-check/send-determinism", "Enable/disable the detection of send-determinism in the communications schemes",
    false, [](bool) {
      _mc_cfg_cb_check("value to enable/disable the detection of send-determinism in the communications schemes");
    }};

simgrid::config::Flag<bool> _sg_mc_debug{
    "model-check/debug",
    "Whether to enable advance runtime verification. Those may be costly and therefore are desactivated by default.",
    false, [](bool) { _mc_cfg_cb_check("value to enable/disable advance runtime verification"); }};

simgrid::config::Flag<bool> _sg_mc_debug_optimality{
    "model-check/debug-optimality",
    "Whether to enable advance runtime verification. Those may be costly and therefore are desactivated by default.",
    false, [](bool) { _mc_cfg_cb_check("value to enable/disable advance runtime verification"); }};

simgrid::config::Flag<bool> _sg_mc_output_lts{"model-check/output-lts",
                                              "Whether to print informations about the explored LTS.", false,
                                              [](bool) { _mc_cfg_cb_check("value to enable/disable LTS printing"); }};
// In a perfect world, We should use the already existing dot output system
// TODO

simgrid::config::Flag<std::string> _sg_mc_buffering{
    "smpi/buffering",
    "Buffering semantic to use for MPI (only used in MC)",
    "infty",
    {{"zero", "No system buffering: MPI_Send is blocking"},
     {"infty", "Infinite system buffering: MPI_Send returns immediately"}},
    [](std::string_view) { _mc_cfg_cb_check("buffering mode"); }};

simgrid::config::Flag<int> _sg_mc_max_depth{"model-check/max-depth", "Maximal exploration depth (default: 1000)", 1000,
                                            [](int) { _mc_cfg_cb_check("max depth value"); }};

simgrid::config::Flag<int> _sg_mc_max_errors{
    "model-check/max-errors",
    "Maximal amount of errors to accept before stopping the exploration (negative value for no maximum).", 0};

simgrid::config::Flag<int> _sg_mc_befs_threshold{
    "model-check/befs-threshold",
    "Percentage of deviation from the best option allowed for the best first search. "
    "The algorithm switches if current * threshold / 100 > best. Use 100 to follow strict strategy and 0 to always "
    "fully explore traces before backtracking."
    "Must be a floating point value between 0 and 100",
    0, [](int val) { xbt_assert(0 <= val and val <= 100); }};

simgrid::config::Flag<bool> _sg_mc_nofork{
    "model-check/no-fork", "Forbids the use of forks to allow the verification of multithreaded applications.", false,
    [](bool val) {
      if (val) {
        _sg_mc_cached_states_interval = 0;
      }
    }};

simgrid::config::Flag<bool> _sg_mc_search_critical_transition{
    "model-check/search-critical",
    "Search for the critical transition. Note that it is disabled by default when asking for multiple errors in "
    "the program.",
    true};

simgrid::config::Flag<int> _sg_mc_soft_timeout{
    "model-check/timeout-soft",
    {"model-check/soft-timeout"},
    "If the exploration lasts more than this timeout (in seconds), gracefully exit at the next backtracking point.",
    -1};

simgrid::mc::ReductionMode simgrid::mc::get_model_checking_reduction()
{
  if (cfg_mc_reduction.get() == "none") {
    return ReductionMode::none;
  } else if (cfg_mc_reduction.get() == "dpor") {
    return ReductionMode::dpor;
  } else if (cfg_mc_reduction.get() == "sdpor") {
    return ReductionMode::sdpor;
  } else if (cfg_mc_reduction.get() == "odpor") {
    return ReductionMode::odpor;
  } else if (cfg_mc_reduction.get() == "udpor") {
    return ReductionMode::udpor;
  } else {
    XBT_INFO("Unknown reduction mode: defaulting to no reduction");
    return ReductionMode::none;
  }
}
