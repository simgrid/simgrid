/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simgrid/sg_config.hpp"
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
      xbt_assert(simgrid::mc::get_model_checking_mode() == simgrid::mc::ModelCheckingMode::NONE ||
                     simgrid::mc::get_model_checking_mode() == simgrid::mc::ModelCheckingMode::REPLAY,
                 "Specifying a MC replay path is not allowed when running the model-checker in mode %s. "
                 "Either remove the model-check/replay parameter, or execute your code out of simgrid-mc.",
                 to_c_str(simgrid::mc::get_model_checking_mode()));
      simgrid::mc::set_model_checking_mode(simgrid::mc::ModelCheckingMode::REPLAY);
      MC_record_path()                 = value;
    }};

simgrid::config::Flag<bool> _sg_mc_timeout{
    "model-check/timeout", "Whether to enable timeouts for wait requests", false, [](bool) {
      _mc_cfg_cb_check("value to enable/disable timeout for wait requests", not MC_record_replay_is_active());
    }};

static simgrid::config::Flag<std::string> cfg_mc_reduction{
    "model-check/reduction", "Specify the kind of exploration reduction (either none or DPOR)", "dpor",
    [](std::string_view value) {
      if (value != "none" && value != "dpor" && value != "sdpor" && value != "odpor" && value != "udpor")
        xbt_die("configuration option 'model-check/reduction' must be one of the following: "
                " 'none', 'dpor', 'sdpor', 'odpor', or 'udpor'");
    }};

simgrid::config::Flag<std::string> _sg_mc_strategy{
    "model-check/strategy",
    "Specify the the kind of heuristic to use for guided model-checking",
    "none",
    {{"none", "No specific strategy: simply pick the first available transition and act as a DFS."},
     {"max_match_comm", "Try to minimize the number of in-fly communication by appairing matching send and receive."},
     {"min_match_comm",
      "Try to maximize the number of in-fly communication by not appairing matching send and receive."},
     {"uniform", "No specific strategy: choices are made randomly based on a uniform sampling."}}};

simgrid::config::Flag<int> _sg_mc_random_seed{"model-check/rand-seed",
                                              "give a specific random seed to initialize the uniform distribution", 0,
                                              [](int) { _mc_cfg_cb_check("Random seed"); }};

simgrid::config::Flag<bool> _sg_mc_comms_determinism{
    "model-check/communications-determinism", "Whether to enable the detection of communication determinism", false,
    [](bool) {
      _mc_cfg_cb_check("value to enable/disable the detection of determinism in the communications schemes");
    }};
simgrid::config::Flag<bool> _sg_mc_send_determinism{
    "model-check/send-determinism",
    "Enable/disable the detection of send-determinism in the communications schemes",
    false,
    [](bool) {
      _mc_cfg_cb_check("value to enable/disable the detection of send-determinism in the communications schemes");
    }};

simgrid::config::Flag<bool> _sg_mc_unfolding_checker{
    "model-check/unfolding-checker",
    "Whether to enable the unfolding-based dynamic partial order reduction to MPI programs", false, [](bool) {
      _mc_cfg_cb_check("value to to enable/disable the unfolding-based dynamic partial order reduction to MPI programs");
    }};

simgrid::config::Flag<std::string> _sg_mc_buffering{
    "smpi/buffering",
    "Buffering semantic to use for MPI (only used in MC)",
    "infty",
    {{"zero", "No system buffering: MPI_Send is blocking"},
     {"infty", "Infinite system buffering: MPI_Send returns immediately"}},
    [](std::string_view) { _mc_cfg_cb_check("buffering mode"); }};

simgrid::config::Flag<int> _sg_mc_max_depth{"model-check/max-depth",
                                            "Maximal exploration depth (default: 1000)",
                                            1000,
                                            [](int) { _mc_cfg_cb_check("max depth value"); }};

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
    XBT_INFO("No reduction will be used: "
             "UDPOR has a dedicated invocation 'model-check/unfolding-checker' "
             "but is not yet fully supported in SimGrid");
    return ReductionMode::none;
  } else {
    XBT_INFO("Unknown reduction mode: defaulting to no reduction");
    return ReductionMode::none;
  }
}
